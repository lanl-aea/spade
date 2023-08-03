#if (defined(HP) && (! defined(HKS_HPUXI)))
#include <iostream.h>                                                           
#else
#include <iostream>

#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd // MSFT "deprecation" warning
#else
    #include <unistd.h>
#endif

using namespace std;
#endif

#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <functional>
#include <ctime>
#include <cctype>
#include <locale>
#include <vector>
#include <iterator>
#include <regex>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include <odb_API.h>
#include <odb_Coupling.h>
#include <odb_MPC.h>
#include <odb_ShellSolidCoupling.h>
#include <odb_MaterialTypes.h>
#include <odb_SectionTypes.h>

#include "H5Cpp.h"
using namespace H5;

#include <odb_extract_object.h>

OdbExtractObject::OdbExtractObject (CmdLineArguments &command_line_arguments, Logging &log_file) {
    log_file.logVerbose("Starting to parse odb file: " + command_line_arguments.getTimeStamp(false));
    odb_String file_name = command_line_arguments["odb-file"].c_str();
    log_file.logDebug("Operating on file:" + command_line_arguments["odb-file"]);

    if (isUpgradeRequiredForOdb(file_name)) {
        log_file.logDebug("Upgrade to odb required.");
        odb_String upgraded_file_name = string("upgraded_" + command_line_arguments["odb-file"]).c_str();
        log_file.log("Upgrading file:" + command_line_arguments["odb-file"]);
        upgradeOdb(file_name, upgraded_file_name);
        file_name = upgraded_file_name;
    }

    try {  // Since the odb object isn't recognized outside the scope of the try/except, block the processing has to be done within the try block
        odb_Odb& odb = openOdb(file_name, true);  // Open as read only
        process_odb(odb, log_file);
        odb.close();
        log_file.logDebug("Odb Parser object successfully created.");
    }
    catch(odb_BaseException& exc) {
        string error_message = exc.UserReport().CStr();
        log_file.logErrorAndExit("odbBaseException caught. Abaqus error message: " + error_message);
    }
    catch(...) {
        log_file.logErrorAndExit("Unkown exception when attempting to open odb file.");
    }


    log_file.logVerbose("Starting to write output file: " + command_line_arguments.getTimeStamp(false));

    if (command_line_arguments["output-file-type"] == "h5") this->write_h5(command_line_arguments, log_file);
    else if (command_line_arguments["output-file-type"] == "json") this->write_json(command_line_arguments, log_file);
    else if (command_line_arguments["output-file-type"] == "yaml") this->write_yaml(command_line_arguments, log_file);

    log_file.logVerbose("Finished writing output file: " + command_line_arguments.getTimeStamp(false));

}

void OdbExtractObject::process_odb(odb_Odb &odb, Logging &log_file) {

    log_file.logVerbose("Reading top level attributes of odb.");
    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = odb.isReadOnly();

    // TODO: potentially figure out way to get amplitudes, filters, or materials

    log_file.logVerbose("Reading odb jobData.");
    odb_JobData jobData = odb.jobData();
    static const char * analysis_code_enum_strings[] = { "Unknown Analysis Code", "Abaqus Standard", "Abaqus Explicit", "Abaqus CFD" };
    this->job_data.analysisCode = analysis_code_enum_strings[jobData.analysisCode()];
    this->job_data.creationTime = jobData.creationTime().CStr();
    this->job_data.machineName = jobData.machineName().CStr();
    this->job_data.modificationTime = jobData.modificationTime().CStr();
    this->job_data.name = jobData.name().CStr();
    static const char * precision_enum_strings[] = { "Single Precision", "Double Precision" };
    this->job_data.precision = precision_enum_strings[jobData.precision()];
    odb_SequenceProductAddOn add_ons = jobData.productAddOns();
    static const char * add_on_enum_strings[] = { "aqua", "design", "biorid", "cel", "soliter", "cavparallel" };
    // Values gotten from: https://help.3ds.com/2023/English/DSSIMULIA_Established/SIMACAEKERRefMap/simaker-c-jobdatacpp.htm?contextscope=all
    // More values found at: /apps/SIMULIA/EstProducts/2023/SMAOdb/PublicInterfaces/odb_Enum.h
    for (int i=0; i<add_ons.size(); i++) {
        this->job_data.productAddOns.push_back(string(add_on_enum_strings[add_ons.constGet(i)]));
    }
    this->job_data.version = jobData.version().CStr();

    if (odb.hasSectorDefinition())
    {
        log_file.logVerbose("Reading odb sector definition.");
	    odb_SectorDefinition sd = odb.sectorDefinition();
        this->sector_definition.numSectors = sd.numSectors();
	    odb_SequenceSequenceFloat symAx = sd.symmetryAxis();
        std::ostringstream ss;
	    ss << "[" << symAx[0][0]<<","<<symAx[0][1]<<","<<symAx[0][2]<<"]";
        this->sector_definition.start_point  = ss.str();
        ss.str(""); ss.clear();  // First line clears the ostringstream, second line clears any error flags
	    ss <<"[" << symAx[1][0]<<","<<symAx[1][1]<<","<<symAx[1][2]<<"]";
        this->sector_definition.end_point  = ss.str();
    } else {
        this->sector_definition.numSectors = 0;
        this->sector_definition.start_point  = "";
        this->sector_definition.end_point  = "";
    }

    log_file.logVerbose("Reading odb section category data.");
    odb_SectionCategoryRepositoryIT section_category_iter(odb.sectionCategories());
    for (section_category_iter.first(); !section_category_iter.isDone(); section_category_iter.next()) {
        odb_SectionCategory section_category = section_category_iter.currentValue();
        this->section_categories.push_back(process_section_category(section_category, log_file));
    }

    log_file.logVerbose("Reading odb user data.");
    odb_UserXYDataRepositoryIT user_xy_data_iter(odb.userData().xyDataObjects());
    for (user_xy_data_iter.first(); !user_xy_data_iter.isDone(); user_xy_data_iter.next()) {
        odb_UserXYData userXYData = user_xy_data_iter.currentValue();
        user_xy_data_type user_xy_data;
        user_xy_data.name = userXYData.name().CStr();
        user_xy_data.sourceDescription = userXYData.sourceDescription().CStr();
        user_xy_data.contentDescription = userXYData.contentDescription().CStr();
        user_xy_data.positionDescription = userXYData.positionDescription().CStr();
        user_xy_data.xAxisLabel = userXYData.xAxisLabel().CStr();
        user_xy_data.yAxisLabel = userXYData.yAxisLabel().CStr();
        user_xy_data.legendLabel = userXYData.legendLabel().CStr();
        user_xy_data.description = userXYData.description().CStr();
        odb_SequenceSequenceFloat user_xy_data_data;
        userXYData.getData(user_xy_data_data);
        int column_number = 0;
        for (int i=0; i<user_xy_data_data.size(); i++) {
            odb_SequenceFloat user_xy_data_data_dimension1 = user_xy_data_data[i];
            if (user_xy_data_data_dimension1.size() > column_number) { column_number = user_xy_data_data_dimension1.size(); } // use the maximum for the number of columns
            vector<float> dimension1;
            for (int j=0; j<user_xy_data_data_dimension1.size(); j++) {
                dimension1.push_back(user_xy_data_data_dimension1[j]);
            }
            user_xy_data.data.push_back(dimension1);
        }
        user_xy_data.max_column_size = column_number;
        this->user_xy_data.push_back(user_xy_data);
    }

    log_file.logVerbose("Reading odb interactions.");
    odb_InteractionRepository interactions = odb.interactions();
    if(interactions.size() > 0) {
        process_interactions (interactions, odb, log_file);
    }

    log_file.logVerbose("Reading odb constraints.");
    odb_ConstraintRepository constraints = odb.constraints();
    if(constraints.size() > 0) {
        process_constraints (constraints, odb, log_file);
    }

    odb_PartRepository& parts = odb.parts();
    odb_PartRepositoryIT parts_iter(parts);    
    for (parts_iter.first(); !parts_iter.isDone(); parts_iter.next()) {
        log_file.logVerbose("Starting to parse part: " + string(parts_iter.currentKey().CStr()));
        odb_Part part = parts[parts_iter.currentKey()];
// TODO: Write code to get parts
    }

// TODO: Write code to get assembly

    odb_InstanceRepository instances = odb.rootAssembly().instances();
    odb_InstanceRepositoryIT instance_iter(instances);
    for (instance_iter.first(); !instance_iter.isDone(); instance_iter.next()) 
    {
        odb_Instance inst = instances[instance_iter.currentKey()];
// TODO: Write code to get instance
    }

    odb_StepRepository step_repository = odb.steps();
    odb_StepRepositoryIT step_iter (step_repository);
    for (step_iter.first(); !step_iter.isDone(); step_iter.next()) 
    {
        odb_Step current_step = step_repository[step_iter.currentKey()];
// TODO: Write code to get steps
    }


}

section_category_type OdbExtractObject::process_section_category (const odb_SectionCategory &section_category, Logging &log_file) {
    section_category_type category;
    category.name = section_category.name().CStr();
    category.description = section_category.description().CStr();
    for (int i=0; i<section_category.sectionPoints().size(); i++) {
        odb_SectionPoint section_point = section_category.sectionPoints(i);
        section_point_type point;
        point.number = to_string(section_point.number());
        point.description = section_point.description().CStr();
        category.sectionPoints.push_back(point);
    }
    return category;
}

tangential_behavior_type OdbExtractObject::process_interaction_property (const odb_InteractionProperty &interaction_property, Logging &log_file) {
//TODO: Some of the data types need to be adjusted (i.e. some are either the string 'NONE' or a float)
    tangential_behavior_type interaction;
    if (odb_isA(odb_ContactProperty, interaction_property)) {
        odb_ContactProperty contact_property = odb_dynamicCast(odb_ContactProperty, interaction_property);
        if (contact_property.hasValue()) {
            odb_TangentialBehavior tangential_behavior = contact_property.tangentialBehavior();
            if (tangential_behavior.hasValue()) {
                interaction.formulation = tangential_behavior.formulation().CStr();
                interaction.directionality = tangential_behavior.directionality().CStr();
                interaction.slipRateDependency = tangential_behavior.slipRateDependency();
                interaction.pressureDependency = tangential_behavior.pressureDependency();
                interaction.temperatureDependency = tangential_behavior.temperatureDependency();
                interaction.dependencies = tangential_behavior.dependencies();
                interaction.exponentialDecayDefinition = tangential_behavior.exponentialDecayDefinition().CStr();

                odb_SequenceSequenceDouble table_data = tangential_behavior.table();
                int r = table_data.size();
                for (int row = 0; row < r; row++) {
                    int column_size = table_data[row].size();
                    vector<double> columns;
                    for (int column = 0; column < column_size; column++) {
                        columns.push_back(table_data[row].constGet(column));
                    }
                    interaction.table.push_back(columns);
                }
                interaction.shearStressLimit = tangential_behavior.shearStressLimit();
                interaction.maximumElasticSlip = tangential_behavior.maximumElasticSlip().CStr();
                interaction.fraction = tangential_behavior.fraction();
                interaction.absoluteDistance = tangential_behavior.absoluteDistance();
                interaction.elasticSlipStiffness = tangential_behavior.elasticSlipStiffness();
                interaction.nStateDependentVars = tangential_behavior.nStateDependentVars();
                interaction.useProperties = tangential_behavior.useProperties();
            }
        }
    } else {
        log_file.logWarning("Unsupported Interaction Property Type");
    }
    return interaction;
}

node_type OdbExtractObject::process_node (const odb_Node &node, Logging &log_file) {
    node_type new_node;
    new_node.label = node.label();
    const float * const coords = node.coordinates();
    new_node.coordinates[0] = coords[0];
    new_node.coordinates[1] = coords[1];
    new_node.coordinates[2] = coords[2];
    return new_node;
}

element_type OdbExtractObject::process_element (const odb_Element &element, Logging &log_file) {
    element_type new_element;
    new_element.label = element.label();
    new_element.type = element.type().CStr();
    int element_connectivity_size;
    const int* const connectivity = element.connectivity(element_connectivity_size); 
    for (int i=0; i < element_connectivity_size; i++) {
        new_element.connectivity.push_back(connectivity[i]);
    }
    odb_SequenceString instance_names = element.instanceNames();
    for (int i=0; i < instance_names.size(); i++) {
        new_element.instanceNames.push_back(instance_names[i].CStr());
    }
    new_element.sectionCategory = process_section_category(element.sectionCategory(), log_file);
    return new_element;
}

set_type OdbExtractObject::process_set (const odb_Set &set, Logging &log_file) {
    set_type new_set;
    if (set.name().empty()) {
        log_file.logVerbose("Empty set.");
        return new_set;
    }

    new_set.name = set.name().CStr();
    switch(set.type()) {
    case odb_Enum::NODE_SET:
        new_set.type = "Node Set";
        break;
    case odb_Enum::ELEMENT_SET:
        new_set.type = "Element Set";
        break;
    case odb_Enum::SURFACE_SET:
        new_set.type = "Surface Set";
        break;
    }

    odb_SequenceString names = set.instanceNames();
    int numInstances = names.size();

    static const char * face_enum_strings[] = { "FACE_UNKNOWN=0", "END1=1", "END2", "END3", "FACE1=11", "FACE2", "FACE3", "FACE4", "FACE5", "FACE6", "EDGE1=101", "EDGE2", "EDGE3", "EDGE4", "EDGE5", "EDGE6", "EDGE7", "EDGE8", "EDGE9", "EDGE10", "EDGE11", "EDGE12", "EDGE13", "EDGE14", "EDGE15", "EDGE16", "EDGE17", "EDGE18", "EDGE19", "EDGE20", "SPOS=1001", "SNEG=1002", "SIDE1=1001", "SIDE2=1002", "DOUBLE_SIDED=1003" };
    
    for (int i=0; i<names.size(); i++) {
        odb_String name = names.constGet(i);        
        new_set.instanceNames.push_back(name.CStr());
        if (new_set.type == "Node Set") {
            const odb_SequenceNode& set_nodes = set.nodes(name);
            for (int n=0; n < set_nodes.size(); n++) {
                new_set.nodes.push_back(process_node(set_nodes.node(n), log_file));
            }	    
        } else if (new_set.type == "Element Set") {
            const odb_SequenceElement& set_elements = set.elements(name);
            for (int n=0; n < set_elements.size(); n++) {
                new_set.elements.push_back(process_element(set_elements.element(n), log_file));
            }	    
        } else if (new_set.type == "Surface Set") {
            const odb_SequenceElement& set_elements = set.elements(name);
            const odb_SequenceElementFace& set_faces = set.faces(name);
            const odb_SequenceNode& set_nodes = set.nodes(name);

            if(set_elements.size() && set_faces.size())
            {
                for (int n=0; n<set_elements.size(); n++) {
                    new_set.elements.push_back(process_element(set_elements.element(n), log_file));
                    new_set.faces.push_back(face_enum_strings[set_faces.constGet(n)]);
                }
            } else if(set_elements.size()) {
                for (int n=0; n < set_elements.size(); n++) {
                    new_set.elements.push_back(process_element(set_elements.element(n), log_file));
                }
            } else {
                for (int n=0; n < set_nodes.size(); n++)  {
                    new_set.nodes.push_back(process_node(set_nodes.node(n), log_file));
                }
            }
	    
        } else {
            log_file.logWarning("Unknown set type.");
        }
    }
    return new_set;
}

void OdbExtractObject::process_interactions (const odb_InteractionRepository &interactions, odb_Odb &odb, Logging &log_file) {
    odb_InteractionRepositoryIT interaction_iter(interactions);
    for (interaction_iter.first(); !interaction_iter.isDone(); interaction_iter.next()) {
        contact_standard_type contact_standard;
        contact_explicit_type contact_explicit;
        odb_Interaction interaction = interaction_iter.currentValue();
        if (odb_isA(odb_SurfaceToSurfaceContactStd,interaction_iter.currentValue())) {
            log_file.logVerbose("Standard Surface To Surface Contact Interaction.");
            odb_SurfaceToSurfaceContactStd sscs = odb_dynamicCast(odb_SurfaceToSurfaceContactStd,interaction_iter.currentValue());

            contact_standard.sliding = sscs.sliding().CStr();
            contact_standard.smooth = sscs.smooth();
            contact_standard.hcrit = sscs.hcrit();
            contact_standard.limitSlideDistance = sscs.limitSlideDistance();
            contact_standard.slideDistance = sscs.slideDistance();
            contact_standard.extensionZone = sscs.extensionZone();
            contact_standard.adjustMethod = sscs.adjustMethod().CStr();
            contact_standard.adjustTolerance = sscs.adjustTolerance();
            contact_standard.enforcement = sscs.enforcement().CStr();
            contact_standard.thickness = sscs.thickness();
            contact_standard.tied = sscs.tied();
            contact_standard.contactTracking = sscs.contactTracking().CStr();
            contact_standard.createStepName = sscs.createStepName().CStr();

            odb_InteractionProperty interaction_property = odb.interactionProperties().constGet(sscs.interactionProperty());
            contact_standard.interactionProperty = process_interaction_property(interaction_property, log_file);

            odb_Set main = sscs.master();
            contact_standard.main = process_set(main, log_file);
            odb_Set secondary = sscs.slave();
            contact_standard.secondary = process_set(secondary, log_file);
            odb_Set adjust = sscs.adjustSet();
            if(!adjust.name().empty()) {
                contact_standard.adjust = process_set(adjust, log_file);
            }

        } else if (odb_isA(odb_SurfaceToSurfaceContactStd,interaction_iter.currentValue())) {
            log_file.logVerbose("Explicit Surface To Surface Contact Interaction.");
    	    odb_SurfaceToSurfaceContactExp ssce = odb_dynamicCast(odb_SurfaceToSurfaceContactExp,interaction_iter.currentValue());
            contact_explicit.sliding = ssce.sliding().CStr();
            contact_explicit.mainNoThick = ssce.masterNoThick();
            contact_explicit.secondaryNoThick = ssce.slaveNoThick();
            contact_explicit.mechanicalConstraint = ssce.mechanicalConstraint().CStr();
            contact_explicit.weightingFactorType = ssce.weightingFactorType().CStr();
            contact_explicit.weightingFactor = ssce.weightingFactor();
            contact_explicit.createStepName = ssce.createStepName().CStr();

            odb_InteractionProperty interaction_property = odb.interactionProperties().constGet(ssce.interactionProperty());
            contact_explicit.interactionProperty = process_interaction_property(interaction_property, log_file);

            contact_explicit.useReverseDatumAxis = ssce.useReverseDatumAxis();
            contact_explicit.contactControls = ssce.contactControls().CStr();
  
            odb_Set main = ssce.master();
            contact_explicit.main = process_set(main, log_file);
            odb_Set secondary = ssce.slave();
            contact_explicit.secondary = process_set(secondary, log_file);

        } else {
              log_file.logWarning("Unsupported Interaction Type.");
        }
        this->standard_interactions.push_back(contact_standard);
        this->explicit_interactions.push_back(contact_explicit);
    }
}

void OdbExtractObject::process_constraints (const odb_ConstraintRepository &constraints, odb_Odb &odb, Logging &log_file) {
    odb_ConstraintRepositoryIT constraint_iter(constraints);
    int constraint_number = 1;

    for (constraint_iter.first(); !constraint_iter.isDone(); constraint_iter.next()) {
        if (odb_isA(odb_Tie,constraint_iter.currentValue())) {
            log_file.logVerbose("Tie Constraint");
            odb_Tie tie = odb_dynamicCast(odb_Tie,constraint_iter.currentValue());
            tie_type processed_tie = process_tie(tie, log_file);
            this->constraints.ties.push_back(processed_tie);
        } else if (odb_isA(odb_DisplayBody,constraint_iter.currentValue())) {
            log_file.logVerbose("Display Body Constraint");
            odb_DisplayBody display_body = odb_dynamicCast(odb_DisplayBody,constraint_iter.currentValue());
            display_body_type processed_display_body = process_display_body(display_body, log_file);
            this->constraints.display_bodies.push_back(processed_display_body);
        } else if (odb_isA(odb_Coupling,constraint_iter.currentValue())) {
            log_file.logVerbose("Coupling Constraint");
            odb_Coupling coupling = odb_dynamicCast(odb_Coupling,constraint_iter.currentValue());
            coupling_type processed_coupling = process_coupling(coupling, log_file);
            this->constraints.couplings.push_back(processed_coupling);
        } else if (odb_isA(odb_MPC,constraint_iter.currentValue())) {
            log_file.logVerbose("MPC Constraint");
            odb_MPC mpc = odb_dynamicCast(odb_MPC,constraint_iter.currentValue());
            mpc_type processed_mpc = process_mpc(mpc, log_file);
            this->constraints.mpc.push_back(processed_mpc);
        } else if (odb_isA(odb_ShellSolidCoupling,constraint_iter.currentValue())) {
            log_file.logVerbose("Shell Solid Coupling Constraint");
            odb_ShellSolidCoupling shell_solid_coupling = odb_dynamicCast(odb_ShellSolidCoupling,constraint_iter.currentValue());
            shell_solid_coupling_type processed_shell_solid_coupling = process_shell_solid_coupling(shell_solid_coupling, log_file);
            this->constraints.shell_solid_couplings.push_back(processed_shell_solid_coupling);
        } else {
            string constraint_name = constraint_iter.currentKey().CStr();
            log_file.logWarning("Unsupported Constraint type for constraint: " + constraint_name);
        }
        constraint_number++;
    }

}

tie_type OdbExtractObject::process_tie (const odb_Tie &tie, Logging &log_file) {
    tie_type new_tie;
    if (tie.hasValue())
    {
        new_tie.main = process_set(tie.master(), log_file);
        new_tie.secondary = process_set(tie.slave(), log_file);
        new_tie.adjust = tie.adjust();
        if (tie.adjust()) {
            new_tie.positionToleranceMethod = tie.positionToleranceMethod().CStr();
            if (new_tie.positionToleranceMethod == "SPECIFIED") {
                new_tie.positionTolerance = tie.positionTolerance();
            }
        }
        new_tie.tieRotations = tie.tieRotations();

        new_tie.constraintRatioMethod = tie.constraintRatioMethod().CStr();
        new_tie.constraintRatio = tie.constraintRatio();
        new_tie.constraintEnforcement = tie.constraintEnforcement().CStr();
        new_tie.thickness = tie.thickness();
    }
    return new_tie;
}
display_body_type OdbExtractObject::process_display_body (const odb_DisplayBody &display_body, Logging &log_file) {
    display_body_type new_display_body;
    if (display_body.hasValue())
    {
        new_display_body.instanceName = display_body.instanceName().CStr();
        new_display_body.referenceNode1InstanceName = display_body.referenceNode1InstanceName().CStr();
        new_display_body.referenceNode1Label = display_body.referenceNode1Label();
        new_display_body.referenceNode2InstanceName = display_body.referenceNode2InstanceName().CStr();
        new_display_body.referenceNode2Label = display_body.referenceNode2Label();
        new_display_body.referenceNode3InstanceName = display_body.referenceNode3InstanceName().CStr();
        new_display_body.referenceNode3Label = display_body.referenceNode3Label();
    }
    return new_display_body;
}
coupling_type OdbExtractObject::process_coupling (const odb_Coupling &coupling, Logging &log_file) {
    coupling_type new_coupling;
    if (coupling.hasValue())
    {
        new_coupling.surface = process_set(coupling.surface(), log_file);
        new_coupling.refPoint = process_set(coupling.refPoint(), log_file);
           
        new_coupling.couplingType = coupling.couplingType().cStr();
        new_coupling.weightingMethod = coupling.weightingMethod().cStr();
        new_coupling.influenceRadius = coupling.influenceRadius();
        new_coupling.u1 = coupling.u1();
        new_coupling.u2 = coupling.u2();
        new_coupling.u3 = coupling.u3();
        new_coupling.ur1 = coupling.ur1();
        new_coupling.ur2 = coupling.ur2();
        new_coupling.ur3 = coupling.ur3();
        new_coupling.nodes = process_set(coupling.couplingNodes(), log_file);
    }
    return new_coupling;
}
mpc_type OdbExtractObject::process_mpc (const odb_MPC &mpc, Logging &log_file) {
    mpc_type new_mpc;
    if (mpc.hasValue())
    {
        new_mpc.surface = process_set(mpc.surface(), log_file);
        new_mpc.refPoint = process_set(mpc.refPoint(), log_file);
        new_mpc.mpcType = mpc.mpcType().cStr();
        new_mpc.userMode = mpc.userMode().cStr();
        new_mpc.userType = mpc.userType();
    }
    return new_mpc;
}
shell_solid_coupling_type OdbExtractObject::process_shell_solid_coupling (const odb_ShellSolidCoupling &shell_solid_coupling, Logging &log_file) {
    shell_solid_coupling_type new_shell_solid_coupling;
	if (shell_solid_coupling.hasValue())
	{
        new_shell_solid_coupling.shellEdge = process_set(shell_solid_coupling.shellEdge(), log_file);
        new_shell_solid_coupling.solidFace = process_set(shell_solid_coupling.solidFace(), log_file);
        new_shell_solid_coupling.positionToleranceMethod = shell_solid_coupling.positionToleranceMethod().cStr();
        new_shell_solid_coupling.positionTolerance = shell_solid_coupling.positionTolerance();
        new_shell_solid_coupling.influenceDistanceMethod = shell_solid_coupling.influenceDistanceMethod().cStr();
        new_shell_solid_coupling.influenceDistance = shell_solid_coupling.influenceDistance();

	}
    return new_shell_solid_coupling;
}

void OdbExtractObject::process_part (const odb_Part &part, odb_Odb &odb, Logging &log_file) {

}
void OdbExtractObject::process_instance (const odb_Instance &instance, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments) {

}
void OdbExtractObject::process_root_assembly (const odb_Assembly &root_assembly, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments) {

}
void OdbExtractObject::process_step (const odb_Step &step, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments) {

}





void OdbExtractObject::write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file) {
// Write out data to hdf5 file

    // Open file for writing
    std::ifstream hdf5File (command_line_arguments["output-file"].c_str());
    log_file.logDebug("Creating hdf5 file " + command_line_arguments["output-file"]);
    const H5std_string FILE_NAME(command_line_arguments["output-file"]);
    H5File h5_file(FILE_NAME, H5F_ACC_TRUNC);

//    H5::Group odb_group = file.createGroup(string("/odb").c_str());
    log_file.logDebug("Creating odb group for meta-data " + command_line_arguments["output-file"]);

//    write_string_dataset(this->odb_group, "name", this->name);
    log_file.logVerbose("Writing top level attributes to odb group.");
    H5::Group odb_group = h5_file.createGroup(string("/odb").c_str());
    write_attribute(odb_group, "name", this->name);
    write_attribute(odb_group, "analysisTitle", this->analysisTitle);
    write_attribute(odb_group, "description", this->description);
    write_attribute(odb_group, "path", this->path);
    stringstream bool_stream; bool_stream << std::boolalpha << this->isReadOnly; string bool_string = bool_stream.str();
    write_attribute(odb_group, "isReadOnly", bool_string);

    log_file.logVerbose("Writing odb jobData.");
    H5::Group job_data_group = h5_file.createGroup(string("/odb/jobData").c_str());
    write_attribute(job_data_group, "analysisCode", this->job_data.analysisCode);
    write_attribute(job_data_group, "creationTime", this->job_data.creationTime);
    write_attribute(job_data_group, "machineName", this->job_data.machineName);
    write_attribute(job_data_group, "modificationTime", this->job_data.modificationTime);
    write_attribute(job_data_group, "name", this->job_data.name);
    write_attribute(job_data_group, "precision", this->job_data.precision);
    write_string_vector_dataset(job_data_group, "productAddOns", this->job_data.productAddOns);
    write_attribute(job_data_group, "version", this->job_data.version);

    log_file.logVerbose("Writing odb sector definition.");
    H5::Group sector_definition_group = h5_file.createGroup(string("/odb/sectorDefinition").c_str());
    write_integer_dataset(sector_definition_group, "numSectors", this->sector_definition.numSectors);
    H5::Group symmetry_axis_group = h5_file.createGroup(string("/odb/sectorDefinition/symmetryAxis").c_str());
    write_string_dataset(symmetry_axis_group, "StartPoint", this->sector_definition.start_point);
    write_string_dataset(symmetry_axis_group, "EndPoint", this->sector_definition.end_point);

    log_file.logVerbose("Writing odb section categories.");
    H5::Group section_categories_group = h5_file.createGroup(string("/odb/sectionCategories").c_str());
    for (int i=0; i<this->section_categories.size(); i++) {
        string category_group_name = "/odb/sectionCategories/" + this->section_categories[i].name;
        H5::Group section_category_group = h5_file.createGroup(category_group_name.c_str());
        write_section_category(h5_file, section_category_group, category_group_name, this->section_categories[i]);
    }

    log_file.logVerbose("Writing odb user data.");
    H5::Group user_data_group = h5_file.createGroup(string("/odb/userData").c_str());
    for (int i=0; i<this->user_xy_data.size(); i++) {
        string user_xy_data_name = "/odb/userData/" + this->user_xy_data[i].name;
        log_file.logDebug("User data name:" + this->user_xy_data[i].name);
        H5::Group user_xy_data_group = h5_file.createGroup(user_xy_data_name.c_str());
        write_string_dataset(user_xy_data_group, "sourceDescription", this->user_xy_data[i].sourceDescription);
        write_string_dataset(user_xy_data_group, "contentDescription", this->user_xy_data[i].contentDescription);
        write_string_dataset(user_xy_data_group, "positionDescription", this->user_xy_data[i].positionDescription);
        write_string_dataset(user_xy_data_group, "xAxisLabel", this->user_xy_data[i].xAxisLabel);
        write_string_dataset(user_xy_data_group, "yAxisLabel", this->user_xy_data[i].yAxisLabel);
        write_string_dataset(user_xy_data_group, "legendLabel", this->user_xy_data[i].legendLabel);
        write_string_dataset(user_xy_data_group, "description", this->user_xy_data[i].description);
        write_float_2D_vector(user_xy_data_group, "data", this->user_xy_data[i].max_column_size, this->user_xy_data[i].data);
    }

    this->contraints_group = h5_file.createGroup(string("/odb/constraints").c_str());
    write_constraints(h5_file, "odb/constraints");
    this->interactions_group = h5_file.createGroup(string("/odb/interactions").c_str());
    this->parts_group = h5_file.createGroup(string("/odb/parts").c_str());
    this->root_assembly_group = h5_file.createGroup(string("/odb/rootAssembly").c_str());
    this->steps_group = h5_file.createGroup(string("/odb/steps").c_str());

//    vector<string> temp_string = { "testing", "this", "vector" };
//    std::vector<const char*> array_of_c_string = { "testing", "this", "vector" };
//    for(const string &group : groups)

    // TODO: potentially add amplitudes group
    // TODO: potentially add filters group
    // TODO: potentially add materials group

    h5_file.close();  // Close the hdf5 file
}

void OdbExtractObject::write_constraints(H5::H5File &h5_file, const string &group_name) {
    if (!this->constraints.ties.empty()) {

    }
    if (!this->constraints.display_bodies.empty()) {

    }
    if (!this->constraints.couplings.empty()) {

    }
    if (!this->constraints.mpc.empty()) {

    }
    if (!this->constraints.shell_solid_couplings.empty()) {

    }
}

void OdbExtractObject::write_element(H5::H5File &h5_file, const string &group_name, const element_type &element) {
    H5::Group element_group = h5_file.createGroup((group_name + "/" + to_string(element.label)).c_str());
    write_string_dataset(element_group, "type", element.type);
    write_integer_vector_dataset(element_group, "connectivity", element.connectivity);
    write_section_category(h5_file, element_group, group_name + "/" + to_string(element.label) + "/sectionCategory", element.sectionCategory);
    write_string_vector_dataset(element_group, "instanceNames", element.instanceNames);
}

void OdbExtractObject::write_elements(H5::H5File &h5_file, const string &group_name, const vector<element_type> &elements) {
    H5::Group elements_group = h5_file.createGroup((group_name + "/elements").c_str());
    for (auto element : elements) { write_element(h5_file, group_name + "/elements", element); }
}

void OdbExtractObject::write_node(H5::H5File &h5_file, const string &group_name, const node_type &node) {
    H5::Group node_group = h5_file.createGroup((group_name + "/" + to_string(node.label)).c_str());
    write_float_array_dataset(node_group, group_name + "/" + to_string(node.label), 3, node.coordinates);
}

void OdbExtractObject::write_nodes(H5::H5File &h5_file, const string &group_name, const vector<node_type> &nodes) {
    H5::Group nodes_group = h5_file.createGroup((group_name + "/nodes").c_str());
    for (auto node : nodes) { write_node(h5_file, group_name + "/nodes", node); }
}

void OdbExtractObject::write_set(H5::H5File &h5_file, const string &group_name, const set_type &set) {
    // TODO
    if (!set.name.empty()) {
        string set_group_name = group_name + "/" + set.name;
        H5::Group set_group = h5_file.createGroup(set_group_name.c_str());
        write_attribute(set_group, "type", set.type);
        write_string_vector_dataset(set_group, "instanceNames", set.instanceNames);
        if (set.type == "Node Set") {
            write_nodes(h5_file, set_group_name, set.nodes);
        } else if (set.type == "Element Set") {
            write_elements(h5_file, set_group_name, set.elements);
        } else if (set.type == "Surface Set") {
            if(!set.elements.empty() && !set.faces.empty())
            {
                write_elements(h5_file, set_group_name, set.elements);
                write_string_vector_dataset(set_group, "faces", set.faces);
            } else if(!set.elements.empty()) {
                write_elements(h5_file, set_group_name, set.elements);
            } else {
                write_nodes(h5_file, set_group_name, set.nodes);
            }
        }
    }
}

void OdbExtractObject::write_section_category(H5::H5File &h5_file, const H5::Group &group, const string &group_name, const section_category_type &section_category) {
    write_string_dataset(group, "description", section_category.description);
    for (int j=0; j<section_category.sectionPoints.size(); j++) {
        string point_group_name = group_name + "/" + section_category.sectionPoints[j].number;
        H5::Group section_point_group = h5_file.createGroup(point_group_name.c_str());
        write_string_dataset(section_point_group, "description", section_category.sectionPoints[j].description);
    }
}

void OdbExtractObject::write_attribute(const H5::Group& group, const string & attribute_name, const string & string_value) {
    H5::DataSpace attribute_space(H5S_SCALAR);
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);  // Use the length of the string or 1 if string is blank
    H5::Attribute attribute = group.createAttribute(attribute_name, string_type, attribute_space);
    attribute.write(string_type, string_value);
    attribute_space.close();
}

void OdbExtractObject::write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one string
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(&string_value[0], string_type);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_string_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<string> & string_values) {
//    hsize_t dimensions[] = {1};
    hsize_t dimensions[1] = {hsize_t(string_values.size())};
    H5::DataSpace dataspace(1, dimensions);  // Create a space for as many strings as are in the vector
//    H5::StrType string_type(0, H5T_VARIABLE);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
    dataset.write(string_values.data(), string_type);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
//    int integer_size = integer_value.size();
//    if (integer_size == 0) { integer_size++; }  // If the integer is empty, make the integer size equal to one, as StrType must have a positive size
//    H5::IntType integer_type (0, integer_size);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::STD_I32BE, dataspace);
    dataset.write(&int_value, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_integer_array_dataset(const H5::Group& group, const string & dataset_name, const int array_size, const int* int_array) {
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
    dataset.write(int_array, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_integer_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<int> & int_data) {
    int int_array[int_data.size()]; // Need to convert vector to array with contiguous memory for H5 to process
    for (int i=0; i<int_data.size(); i++) { int_array[i] = int_data[i]; }
    write_integer_array_dataset(group, dataset_name, int_data.size(), int_array);
}

void OdbExtractObject::write_float_dataset(const H5::Group &group, const string &dataset_name, const float &float_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(&float_value, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_float_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const float* float_array) {
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_float_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<float> &float_data) {
    float float_array[float_data.size()]; // Need to convert vector to array with contiguous memory for H5 to process
    for (int i=0; i<float_data.size(); i++) { float_array[i] = float_data[i]; }
    write_float_array_dataset(group, dataset_name, float_data.size(), float_array);
}

void OdbExtractObject::write_float_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, float** &float_array) {
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void OdbExtractObject::write_float_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, vector<vector<float>> & float_data) {
    float float_array[float_data.size()][max_column_size]; // Need to convert vector to array with contiguous memory for H5 to process
    for( int i = 0; i<float_data.size(); ++i) {
        for( int j = 0; j<float_data[i].size(); ++j) {
            float_array[i][j] = float_data[i][j];
        }
    }
    write_float_2D_array(group, dataset_name, float_data.size(), max_column_size, *float_array);
}


void OdbExtractObject::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file) {
}

void OdbExtractObject::write_json (CmdLineArguments &command_line_arguments, Logging &log_file) {
}