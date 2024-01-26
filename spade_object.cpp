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
#include <filesystem>

#include <odb_API.h>
#include <odb_Coupling.h>
#include <odb_MPC.h>
#include <odb_ShellSolidCoupling.h>
#include <odb_MaterialTypes.h>
#include <odb_SectionTypes.h>

#include "H5Cpp.h"
using namespace H5;

#include <spade_object.h>

SpadeObject::SpadeObject (CmdLineArguments &command_line_arguments, Logging &log_file) {
    log_file.logVerbose("Starting to read odb file: " + command_line_arguments.getTimeStamp(false));
    odb_String file_name = command_line_arguments["odb-file"].c_str();
    log_file.logDebug("Operating on file:" + command_line_arguments["odb-file"]);

    if (isUpgradeRequiredForOdb(file_name)) {
        log_file.logDebug("Upgrade to odb required.");
        odb_String upgraded_file_name = std::filesystem::path(command_line_arguments["odb-file"]).replace_extension(".upgraded.odb").generic_string().c_str();
        log_file.log("Upgrading file:" + command_line_arguments["odb-file"]);
        upgradeOdb(file_name, upgraded_file_name);
        file_name = upgraded_file_name;
    }
    // Set up enum strings to be used with multiple functions later
    this->dimension_enum_strings[0] = "Unknown Dimension";
    this->dimension_enum_strings[1] = "Three Dimensional";
    this->dimension_enum_strings[2] = "Two Dimensional Planar";
    this->dimension_enum_strings[3] = "AxiSymmetric";
    this->faces_enum_strings[0] = "FACE_UNKNOWN=0";
    this->faces_enum_strings[1] = "END1=1";
    this->faces_enum_strings[2] = "END2";
    this->faces_enum_strings[3] = "END3";
    this->faces_enum_strings[4] = "FACE1=11";
    this->faces_enum_strings[5] = "FACE2";
    this->faces_enum_strings[6] = "FACE3";
    this->faces_enum_strings[7] = "FACE4";
    this->faces_enum_strings[8] = "FACE5";
    this->faces_enum_strings[9] = "FACE6";
    this->faces_enum_strings[10] = "EDGE1=101";
    this->faces_enum_strings[11] = "EDGE2";
    this->faces_enum_strings[12] = "EDGE3";
    this->faces_enum_strings[13] = "EDGE4";
    this->faces_enum_strings[14] = "EDGE5";
    this->faces_enum_strings[15] = "EDGE6";
    this->faces_enum_strings[16] = "EDGE7";
    this->faces_enum_strings[17] = "EDGE8";
    this->faces_enum_strings[18] = "EDGE9";
    this->faces_enum_strings[19] = "EDGE10";
    this->faces_enum_strings[20] = "EDGE11";
    this->faces_enum_strings[21] = "EDGE12";
    this->faces_enum_strings[22] = "EDGE13";
    this->faces_enum_strings[23] = "EDGE14";
    this->faces_enum_strings[24] = "EDGE15";
    this->faces_enum_strings[25] = "EDGE16";
    this->faces_enum_strings[26] = "EDGE17";
    this->faces_enum_strings[27] = "EDGE18";
    this->faces_enum_strings[28] = "EDGE19";
    this->faces_enum_strings[29] = "EDGE20";
    this->faces_enum_strings[30] = "SPOS=1001";
    this->faces_enum_strings[31] = "SNEG=1002";
    this->faces_enum_strings[32] = "SIDE1=1001"; // = SPOS
    this->faces_enum_strings[33] = "SIDE2=1002"; // = SNEG
    this->faces_enum_strings[34] = "DOUBLE_SIDED=1003"; // = DOUBLE SIDED SHELLS

    this->command_line_arguments = &command_line_arguments;
    try {  // Since the odb object isn't recognized outside the scope of the try/except, block the processing has to be done within the try block
        odb_Odb& odb = openOdb(file_name, true);  // Open as read only
        process_odb(odb, log_file, command_line_arguments);
        odb.close();
        log_file.logDebug("Odb Extract object successfully created.");
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

void SpadeObject::process_odb(odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments) {

    log_file.logVerbose("Reading top level attributes of odb.");
    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = (odb.isReadOnly()) ? "true" : "false";

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
        log_file.logVerbose("Starting to read part: " + string(parts_iter.currentKey().CStr()));
        odb_Part part = parts[parts_iter.currentKey()];
        log_file.logDebug("Part: " + string(part.name().CStr()));
        log_file.logDebug("\tnodes size: " + to_string(part.nodes().size()));
        log_file.logDebug("\telements size: " + to_string(part.elements().size()));
        this->parts.push_back(process_part(part, odb, log_file));
    }

    log_file.logVerbose("Reading root assembly.");
    this->root_assembly = process_assembly(odb.rootAssembly(), odb, log_file);

    log_file.logVerbose("Reading steps.");
    odb_StepRepository step_repository = odb.steps();
    odb_StepRepositoryIT step_iter (step_repository);
    for (step_iter.first(); !step_iter.isDone(); step_iter.next()) 
    {
        const odb_Step& current_step = step_repository[step_iter.currentKey()];
        process_step(current_step, odb, log_file, command_line_arguments);
    cout << "two" << endl;
 cout << &this->steps[0].frames[1].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << &this->steps[0].frames[2].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << this->steps[0].frames[1].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << this->steps[0].frames[2].fieldOutputs[2].dataValues[0].data[0] << endl;
    }
    cout << "three" << endl;
 cout << &this->steps[0].frames[1].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << &this->steps[0].frames[2].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << this->steps[0].frames[1].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << this->steps[0].frames[2].fieldOutputs[2].dataValues[0].data[0] << endl;
    /*
    */


}

section_category_type SpadeObject::process_section_category (const odb_SectionCategory &section_category, Logging &log_file) {
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

tangential_behavior_type SpadeObject::process_interaction_property (const odb_InteractionProperty &interaction_property, Logging &log_file) {
    tangential_behavior_type interaction;
    if (odb_isA(odb_ContactProperty, interaction_property)) {
        odb_ContactProperty contact_property = odb_dynamicCast(odb_ContactProperty, interaction_property);
        if (contact_property.hasValue()) {
            odb_TangentialBehavior tangential_behavior = contact_property.tangentialBehavior();
            if (tangential_behavior.hasValue()) {
                interaction.formulation = tangential_behavior.formulation().CStr();
                interaction.directionality = tangential_behavior.directionality().CStr();
                interaction.slipRateDependency = (tangential_behavior.slipRateDependency()) ? "true" : "false";
                interaction.pressureDependency = (tangential_behavior.pressureDependency()) ? "true" : "false";
                interaction.temperatureDependency = (tangential_behavior.temperatureDependency()) ? "true" : "false";
                interaction.dependencies = tangential_behavior.dependencies();
                interaction.exponentialDecayDefinition = tangential_behavior.exponentialDecayDefinition().CStr();

                odb_SequenceSequenceDouble table_data = tangential_behavior.table();
                interaction.max_column_size = 0;
                int r = table_data.size();
                for (int row = 0; row < r; row++) {
                    int column_size = table_data[row].size();
                    if (column_size > interaction.max_column_size) { interaction.max_column_size = column_size; }
                    vector<double> columns;
                    for (int column = 0; column < column_size; column++) {
                        columns.push_back(table_data[row].constGet(table_data[row].constGet(column)));
                    }
                    interaction.table.push_back(columns);
                }
                interaction.shearStressLimit = tangential_behavior.shearStressLimit();
                interaction.maximumElasticSlip = tangential_behavior.maximumElasticSlip().CStr();
                interaction.fraction = tangential_behavior.fraction();
                interaction.absoluteDistance = tangential_behavior.absoluteDistance();
                interaction.elasticSlipStiffness = tangential_behavior.elasticSlipStiffness();
                interaction.nStateDependentVars = tangential_behavior.nStateDependentVars();
                interaction.useProperties = (tangential_behavior.useProperties()) ? "true" : "false";
            }
        }
    } else {
        log_file.logWarning("Unsupported Interaction Property Type");
    }
    return interaction;
}

node_type* SpadeObject::process_node(const odb_Node &node, Logging &log_file) {
    std::stringstream floatS;
    floatS << std::noshowpos << std::setprecision(7) << node.label() << "_" << node.coordinates()[0] << "_" << node.coordinates()[0] << "_" << node.coordinates()[0];
    string node_key(floatS.str());  // Needed to set the maximum precision of a float (7) to make sure the string is unique
    try {  // If the node has been stored in nodes, just return the address to it
        return &this->nodes.at(node_key);  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
    } catch (const std::out_of_range& oor) {
        node_type new_node;
        new_node.label = node.label();
        new_node.coordinates[0] = node.coordinates()[0];
        new_node.coordinates[1] = node.coordinates()[1];
        new_node.coordinates[2] = node.coordinates()[2];
        log_file.logDebug("\t\tnode " + node_key);
        this->nodes[node_key] = new_node;
        return &this->nodes[node_key];
    }
}

element_type* SpadeObject::process_element(const odb_Element &element, Logging &log_file) {
    // TODO: consider another way of uniquely identifying an element, since the string with all the instance names can be quite long
    element_type new_element;
    string element_key;
    new_element.label = element.label();
    odb_SequenceString instance_names = element.instanceNames();
    for (int i=0; i < instance_names.size(); i++) {
        new_element.instanceNames.push_back(instance_names[i].CStr());
        element_key += new_element.instanceNames[i];
    }
    element_key = to_string(new_element.label) + element_key;
    try {
        return &this->elements.at(element_key);  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
    } catch (const std::out_of_range& oor) {
        new_element.type = element.type().CStr();
        int element_connectivity_size;
        const int* const connectivity = element.connectivity(element_connectivity_size); 
        for (int i=0; i < element_connectivity_size; i++) {
            new_element.connectivity.push_back(connectivity[i]);
        }
        log_file.logDebug("\t\telement " + to_string(new_element.label) + ": connectivity size: " + to_string(new_element.connectivity.size()) + " instances size:" + to_string(new_element.instanceNames.size()));
        new_element.sectionCategory = process_section_category(element.sectionCategory(), log_file);
        this->elements[element_key] = new_element;
        return &this->elements[element_key];
    }
}

set_type SpadeObject::process_set (const odb_Set &set, Logging &log_file) {
    set_type new_set;
    if (set.name().empty()) {
        log_file.logVerbose("Empty set.");
        return new_set;
    }

    new_set.name = set.name().CStr();
    switch(set.type()) {
        case odb_Enum::NODE_SET: new_set.type = "Node Set"; break;
        case odb_Enum::ELEMENT_SET: new_set.type = "Element Set"; break;
        case odb_Enum::SURFACE_SET: new_set.type = "Surface Set"; break;
    }
    log_file.logDebug("\t\tset " + new_set.name + ": " + new_set.type);

    odb_SequenceString names = set.instanceNames();
    int numInstances = names.size();

    for (int i=0; i<names.size(); i++) {
        odb_String name = names.constGet(i);        
        log_file.logDebug("\t\t\tinstance: " + string(name.CStr()));
        new_set.instanceNames.push_back(name.CStr());
        if (new_set.type == "Node Set") {
            const odb_SequenceNode& set_nodes = set.nodes(name);
            for (int n=0; n < set_nodes.size(); n++) { new_set.nodes.push_back(process_node(set_nodes.node(n), log_file)); }
        } else if (new_set.type == "Element Set") {
            const odb_SequenceElement& set_elements = set.elements(name);
            for (int n=0; n < set_elements.size(); n++) { new_set.elements.push_back(process_element(set_elements.element(n), log_file)); }
        } else if (new_set.type == "Surface Set") {
            const odb_SequenceElement& set_elements = set.elements(name);
            const odb_SequenceElementFace& set_faces = set.faces(name);
            const odb_SequenceNode& set_nodes = set.nodes(name);

            if(set_elements.size() && set_faces.size())
            {
                for (int n=0; n<set_elements.size(); n++) {
                    new_set.elements.push_back(process_element(set_elements.element(n), log_file));
                    new_set.faces.push_back(this->faces_enum_strings[set_faces.constGet(n)]);
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

void SpadeObject::process_interactions (const odb_InteractionRepository &interactions, odb_Odb &odb, Logging &log_file) {
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
            contact_standard.limitSlideDistance = (sscs.limitSlideDistance()) ? "true" : "false";
            contact_standard.slideDistance = sscs.slideDistance();
            contact_standard.extensionZone = sscs.extensionZone();
            contact_standard.adjustMethod = sscs.adjustMethod().CStr();
            contact_standard.adjustTolerance = sscs.adjustTolerance();
            contact_standard.enforcement = sscs.enforcement().CStr();
            contact_standard.thickness = (sscs.thickness()) ? "true" : "false";
            contact_standard.tied = (sscs.tied()) ? "true" : "false";
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
            contact_explicit.mainNoThick = (ssce.masterNoThick()) ? "true" : "false";
            contact_explicit.secondaryNoThick = (ssce.slaveNoThick()) ? "true" : "false";
            contact_explicit.mechanicalConstraint = ssce.mechanicalConstraint().CStr();
            contact_explicit.weightingFactorType = ssce.weightingFactorType().CStr();
            contact_explicit.weightingFactor = ssce.weightingFactor();
            contact_explicit.createStepName = ssce.createStepName().CStr();

            odb_InteractionProperty interaction_property = odb.interactionProperties().constGet(ssce.interactionProperty());
            contact_explicit.interactionProperty = process_interaction_property(interaction_property, log_file);

            contact_explicit.useReverseDatumAxis = (ssce.useReverseDatumAxis()) ? "true" : "false";
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

void SpadeObject::process_constraints (const odb_ConstraintRepository &constraints, odb_Odb &odb, Logging &log_file) {
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

tie_type SpadeObject::process_tie (const odb_Tie &tie, Logging &log_file) {
    tie_type new_tie;
    if (tie.hasValue())
    {
        new_tie.main = process_set(tie.master(), log_file);
        new_tie.secondary = process_set(tie.slave(), log_file);
        new_tie.adjust = (tie.adjust()) ? "true" : "false";
        if (tie.adjust()) {
            new_tie.positionToleranceMethod = tie.positionToleranceMethod().CStr();
            if (new_tie.positionToleranceMethod == "SPECIFIED") {
                new_tie.positionTolerance = tie.positionTolerance();
            }
        }
        new_tie.tieRotations = (tie.tieRotations()) ? "true" : "false";

        new_tie.constraintRatioMethod = tie.constraintRatioMethod().CStr();
        new_tie.constraintRatio = tie.constraintRatio();
        new_tie.constraintEnforcement = tie.constraintEnforcement().CStr();
        new_tie.thickness = tie.thickness();
    }
    return new_tie;
}
display_body_type SpadeObject::process_display_body (const odb_DisplayBody &display_body, Logging &log_file) {
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
coupling_type SpadeObject::process_coupling (const odb_Coupling &coupling, Logging &log_file) {
    coupling_type new_coupling;
    if (coupling.hasValue())
    {
        new_coupling.surface = process_set(coupling.surface(), log_file);
        new_coupling.refPoint = process_set(coupling.refPoint(), log_file);
           
        new_coupling.couplingType = coupling.couplingType().cStr();
        new_coupling.weightingMethod = coupling.weightingMethod().cStr();
        new_coupling.influenceRadius = coupling.influenceRadius();
        new_coupling.u1 = (coupling.u1()) ? "true" : "false";
        new_coupling.u2 = (coupling.u2()) ? "true" : "false";
        new_coupling.u3 = (coupling.u3()) ? "true" : "false";
        new_coupling.ur1 = (coupling.ur1()) ? "true" : "false";
        new_coupling.ur2 = (coupling.ur2()) ? "true" : "false";
        new_coupling.ur3 = (coupling.ur3()) ? "true" : "false";
        new_coupling.nodes = process_set(coupling.couplingNodes(), log_file);
    }
    return new_coupling;
}
mpc_type SpadeObject::process_mpc (const odb_MPC &mpc, Logging &log_file) {
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
shell_solid_coupling_type SpadeObject::process_shell_solid_coupling (const odb_ShellSolidCoupling &shell_solid_coupling, Logging &log_file) {
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

part_type SpadeObject::process_part (const odb_Part &part, odb_Odb &odb, Logging &log_file) {
    part_type new_part;
    new_part.name = part.name().CStr();
    new_part.embeddedSpace = this->dimension_enum_strings[part.embeddedSpace()];

    const odb_SequenceNode& nodes = part.nodes();
    for (int i=0; i<nodes.size(); i++)  { new_part.nodes.push_back(process_node(nodes.node(i), log_file)); }
    odb_SequenceElement elements = part.elements();
    for (int i=0; i<elements.size(); i++)  { new_part.elements.push_back(process_element(elements.element(i), log_file)); }

    odb_SetRepositoryIT node_iter(part.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_part.nodeSets.push_back(process_set(node_iter.currentValue(), log_file));
    }
    odb_SetRepositoryIT element_iter(part.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_part.elementSets.push_back(process_set(element_iter.currentValue(), log_file));
    }
    odb_SetRepositoryIT surface_iter(part.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_part.surfaces.push_back(process_set(surface_iter.currentValue(), log_file));
    }

    return new_part;
}

section_assignment_type SpadeObject::process_section_assignment (const odb_SectionAssignment &section_assignment, Logging &log_file) {
    section_assignment_type new_section_assignment;
    new_section_assignment.region = process_set(section_assignment.region(), log_file);
    new_section_assignment.sectionName = section_assignment.sectionName().CStr();
    return new_section_assignment;
}

beam_orientation_type SpadeObject::process_beam_orientation (const odb_BeamOrientation &beam_orientation, Logging &log_file) {
    beam_orientation_type new_beam_orientation;
    switch(beam_orientation.method()) {
        case odb_Enum::N1_COSINES: new_beam_orientation.method = "N1 Cosines"; break;
        case odb_Enum::CSYS: new_beam_orientation.method = "Csys"; break;
        case odb_Enum::VECT: new_beam_orientation.method = "Vector"; break;
    }
    new_beam_orientation.region = process_set(beam_orientation.region(), log_file);
    odb_SequenceFloat beam_vector = beam_orientation.vector();
    for (int i=0; i<beam_vector.size(); i++) {
        new_beam_orientation.beam_vector.push_back(beam_vector[i]);
    }
    return new_beam_orientation;
}

rebar_orientation_type SpadeObject::process_rebar_orientation (const odb_RebarOrientation &rebar_orientation, Logging &log_file) {
    rebar_orientation_type new_rebar_orientation;
    switch(rebar_orientation.axis()) {
        case odb_Enum::AXIS_1: new_rebar_orientation.axis = "Axis 1"; break;
        case odb_Enum::AXIS_2: new_rebar_orientation.axis = "Axis 2"; break;
        case odb_Enum::AXIS_3: new_rebar_orientation.axis = "Axis 3"; break;
    }
    new_rebar_orientation.angle = rebar_orientation.angle();
    new_rebar_orientation.region = process_set(rebar_orientation.region(), log_file);
    new_rebar_orientation.csys = process_csys(rebar_orientation.csys(), log_file);

    return new_rebar_orientation;
}

datum_csys_type SpadeObject::process_csys(const odb_DatumCsys &datum_csys, Logging &log_file) {
    datum_csys_type new_datum_csys;
    new_datum_csys.name = datum_csys.name().CStr();
    switch(datum_csys.type()) {
        case odb_Enum::CARTESIAN: new_datum_csys.type = "Cartesian"; break;
        case odb_Enum::CYLINDRICAL: new_datum_csys.type = "Cylindrical"; break;
        case odb_Enum::SPHERICAL: new_datum_csys.type = "Spherical"; break;
    }
    const float* x_axis = datum_csys.xAxis();
    const float* y_axis = datum_csys.yAxis();
    const float* z_axis = datum_csys.zAxis();
    const float* origin = datum_csys.origin();
    new_datum_csys.x_axis[0] = x_axis[0]; new_datum_csys.x_axis[1] = x_axis[1]; new_datum_csys.x_axis[2] = x_axis[2];
    new_datum_csys.y_axis[0] = y_axis[0]; new_datum_csys.y_axis[1] = y_axis[1]; new_datum_csys.y_axis[2] = y_axis[2];
    new_datum_csys.z_axis[0] = z_axis[0]; new_datum_csys.z_axis[1] = z_axis[1]; new_datum_csys.z_axis[2] = z_axis[2];
    new_datum_csys.origin[0] = origin[0]; new_datum_csys.origin[1] = origin[1]; new_datum_csys.origin[2] = origin[2];
    return new_datum_csys;
}
analytic_surface_segment_type SpadeObject::process_segment (const odb_AnalyticSurfaceSegment &segment, Logging &log_file) {
    analytic_surface_segment_type new_segment;
    new_segment.type = segment.type().CStr();
    odb_SequenceSequenceFloat segment_data = segment.data();
    int column_number = 0;
    for (int i=0; i<segment_data.size(); i++) {
        odb_SequenceFloat segment_data_dimension1 = segment_data[i];
        if (segment_data_dimension1.size() > column_number) { column_number = segment_data_dimension1.size(); }
        vector<float> dimension1;
        for (int j=0; j<segment_data_dimension1.size(); j++) {
            dimension1.push_back(segment_data_dimension1[j]);
        }
        new_segment.data.push_back(dimension1);
    }
    new_segment.max_column_size = column_number;
    return new_segment;
}

analytic_surface_type SpadeObject::process_analytic_surface (const odb_AnalyticSurface &analytic_surface, Logging &log_file) {
    analytic_surface_type new_analytic_surface;
    new_analytic_surface.name = analytic_surface.name().CStr();
    new_analytic_surface.type = analytic_surface.type().CStr();
    new_analytic_surface.filletRadius = analytic_surface.filletRadius();
    const odb_SequenceAnalyticSurfaceSegment& segments = analytic_surface.segments();
    for (int i=0; i<segments.size(); i++)  { new_analytic_surface.segments.push_back(process_segment(segments[i], log_file)); }
    odb_SequenceSequenceFloat coord_data = analytic_surface.localCoordData();
    int column_number = 0;
    for (int i=0; i<coord_data.size(); i++) {
        odb_SequenceFloat coord_data_dimension1 = coord_data[i];
        if (coord_data_dimension1.size() > column_number) { column_number = coord_data_dimension1.size(); }
        vector<float> dimension1;
        for (int j=0; j<coord_data_dimension1.size(); j++) {
            dimension1.push_back(coord_data_dimension1[j]);
        }
        new_analytic_surface.localCoordData.push_back(dimension1);
    }
    new_analytic_surface.max_column_size = column_number;
    return new_analytic_surface;
}

rigid_body_type SpadeObject::process_rigid_body (const odb_RigidBody &rigid_body, Logging &log_file) {
    rigid_body_type new_rigid_body;
    new_rigid_body.position = rigid_body.position().CStr();
    new_rigid_body.isothermal = (rigid_body.isothermal()) ? "true" : "false";
    try { new_rigid_body.referenceNode = process_set(rigid_body.referenceNodeSet(), log_file);
    } catch(odb_BaseException& e) { log_file.logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.elements = process_set(rigid_body.elements(), log_file);
    } catch(odb_BaseException& e) { log_file.logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.tieNodes = process_set(rigid_body.tieNodes(), log_file);
    } catch(odb_BaseException& e) { log_file.logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.pinNodes = process_set(rigid_body.pinNodes(), log_file);
    } catch(odb_BaseException& e) { log_file.logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.analyticSurface = process_analytic_surface(rigid_body.analyticSurface(), log_file);
    } catch(odb_BaseException& e) { log_file.logWarning(e.UserReport().CStr()); }
    return new_rigid_body;
}

instance_type SpadeObject::process_instance (const odb_Instance &instance, odb_Odb &odb, Logging &log_file) {
    instance_type new_instance;
    new_instance.name = instance.name().CStr();
    new_instance.embeddedSpace = this->dimension_enum_strings[instance.embeddedSpace()];

    log_file.logDebug("Instance: " + new_instance.name);
    log_file.logDebug("\tnodes size: " + to_string(instance.nodes().size()));
    log_file.logDebug("\telements size: " + to_string(instance.elements().size()));

    const odb_SequenceNode& nodes = instance.nodes();
    for (int i=0; i<nodes.size(); i++)  { new_instance.nodes.push_back(process_node(nodes.node(i), log_file)); }
    odb_SequenceElement elements = instance.elements();
    for (int i=0; i<elements.size(); i++)  { new_instance.elements.push_back(process_element(elements.element(i), log_file)); }

    odb_SetRepositoryIT node_iter(instance.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_instance.nodeSets.push_back(process_set(node_iter.currentValue(), log_file));
    }
    log_file.logDebug("\tnodeSets size: " + to_string(new_instance.nodeSets.size()));
    odb_SetRepositoryIT element_iter(instance.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_instance.elementSets.push_back(process_set(element_iter.currentValue(), log_file));
    }
    log_file.logDebug("\telementSets size: " + to_string(new_instance.elementSets.size()));
    odb_SetRepositoryIT surface_iter(instance.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_instance.surfaces.push_back(process_set(surface_iter.currentValue(), log_file));
    }
    log_file.logDebug("\tsurfaces size: " + to_string(new_instance.surfaces.size()));
    const odb_SequenceRigidBody& rigid_bodies = instance.rigidBodies();
    for (int i=0; i<rigid_bodies.size(); i++)  { new_instance.rigidBodies.push_back(process_rigid_body(rigid_bodies[i], log_file)); }
    const odb_SequenceSectionAssignment& section_assignments = instance.sectionAssignments();
    for (int i=0; i<section_assignments.size(); i++)  { new_instance.sectionAssignments.push_back(process_section_assignment(section_assignments[i], log_file)); }
    const odb_SequenceBeamOrientation& beam_orientations = instance.beamOrientations();
    for (int i=0; i<beam_orientations.size(); i++)  { new_instance.beamOrientations.push_back(process_beam_orientation(beam_orientations[i], log_file)); }
    const odb_SequenceRebarOrientation& rebar_orientations = instance.rebarOrientations();
    for (int i=0; i<rebar_orientations.size(); i++)  { new_instance.rebarOrientations.push_back(process_rebar_orientation(rebar_orientations[i], log_file)); }
    try {
        new_instance.analyticSurface = process_analytic_surface(instance.analyticSurface(), log_file);
    } catch(odb_BaseException& e) {
        log_file.logWarning(e.UserReport().CStr());
    }
    return new_instance;
}

connector_orientation_type SpadeObject::process_connector_orientation (const odb_ConnectorOrientation &connector_orientation, Logging &log_file) {
    connector_orientation_type new_connector_orientation;
    new_connector_orientation.region = process_set(connector_orientation.region(), log_file);
    switch(connector_orientation.axis1()) {
        case odb_Enum::AXIS_1: new_connector_orientation.axis1 = "Axis 1"; break;
        case odb_Enum::AXIS_2: new_connector_orientation.axis1 = "Axis 2"; break;
        case odb_Enum::AXIS_3: new_connector_orientation.axis1 = "Axis 3"; break;
    }
    switch(connector_orientation.axis2()) {
        case odb_Enum::AXIS_1: new_connector_orientation.axis2 = "Axis 1"; break;
        case odb_Enum::AXIS_2: new_connector_orientation.axis2 = "Axis 2"; break;
        case odb_Enum::AXIS_3: new_connector_orientation.axis2 = "Axis 3"; break;
    }
    new_connector_orientation.orient2sameAs1 = (connector_orientation.orient2sameAs1()) ? "true" : "false";
    new_connector_orientation.angle1 = connector_orientation.angle1();
    new_connector_orientation.angle2 = connector_orientation.angle2();
    new_connector_orientation.localCsys1 = process_csys(connector_orientation.csys1(), log_file);
    new_connector_orientation.localCsys2 = process_csys(connector_orientation.csys2(), log_file);
    return new_connector_orientation;
}

assembly_type SpadeObject::process_assembly (odb_Assembly &assembly, odb_Odb &odb, Logging &log_file) {
    assembly_type new_assembly;
    new_assembly.name = assembly.name().CStr();
    log_file.logVerbose("Reading assembly data for " + new_assembly.name);
    new_assembly.embeddedSpace = this->dimension_enum_strings[assembly.embeddedSpace()];

    log_file.logDebug("Assembly: " + new_assembly.name);
    log_file.logDebug("\tnodes size: " + to_string(assembly.nodes().size()));
    log_file.logDebug("\telements size: " + to_string(assembly.elements().size()));

    const odb_SequenceNode& nodes = assembly.nodes();
    for (int i=0; i<nodes.size(); i++)  { new_assembly.nodes.push_back(process_node(nodes.node(i), log_file)); }
    const odb_SequenceElement& elements = assembly.elements();
    for (int i=0; i<elements.size(); i++)  { new_assembly.elements.push_back(process_element(elements.element(i), log_file)); }

    log_file.logDebug("\tnodeSets:");
    odb_SetRepositoryIT node_iter(assembly.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_assembly.nodeSets.push_back(process_set(node_iter.currentValue(), log_file));
    }
    log_file.logDebug("\tnodeSets size " + to_string(new_assembly.nodeSets.size()));
    log_file.logDebug("\telementSets:");
    odb_SetRepositoryIT element_iter(assembly.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_assembly.elementSets.push_back(process_set(element_iter.currentValue(), log_file));
    }
    log_file.logDebug("\telementSets size " + to_string(new_assembly.elementSets.size()));
    log_file.logDebug("\tsurfaces:");
    odb_SetRepositoryIT surface_iter(assembly.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_assembly.surfaces.push_back(process_set(surface_iter.currentValue(), log_file));
    }
    log_file.logDebug("\tsurfaces size " + to_string(new_assembly.surfaces.size()));

    odb_InstanceRepository instances = assembly.instances();
    odb_InstanceRepositoryIT instance_iter(instances);
    for (instance_iter.first(); !instance_iter.isDone(); instance_iter.next()) {
        log_file.logVerbose("Starting to read instance: " + string(instance_iter.currentKey().CStr()));
        const odb_Instance& instance = instances[instance_iter.currentKey()];
        new_assembly.instances.push_back(process_instance(instance, odb, log_file));
    }
    odb_DatumCsysRepository datum_csyses = assembly.datumCsyses();
    odb_DatumCsysRepositoryIT datum_csyses_iter(datum_csyses);
    for (datum_csyses_iter.first(); !datum_csyses_iter.isDone(); datum_csyses_iter.next()) {
        const odb_DatumCsys& datum_csys = datum_csyses[datum_csyses_iter.currentKey()];
        new_assembly.datumCsyses.push_back(process_csys(datum_csys, log_file));
    }
    // TODO: Maybe reach out to 3DS to determine if they plan to implement 'odb_SequencePretensionSection' as it is declared, but not defined
//    const odb_SequencePretensionSection& pretension_sections = assembly.pretensionSections();
    const odb_SequenceConnectorOrientation& connector_orientations = assembly.connectorOrientations();
    for (int i=0; i<connector_orientations.size(); i++)  { new_assembly.connectorOrientations.push_back(process_connector_orientation(connector_orientations[i], log_file)); }
    return new_assembly;
}

field_value_type SpadeObject::process_field_values(const odb_FieldValue &field_value, const odb_SequenceInvariant& invariants, Logging &log_file, CmdLineArguments &command_line_arguments) {
    field_value_type new_field_value;
    new_field_value.empty = true;
    new_field_value.elementLabel = field_value.elementLabel();
    new_field_value.nodeLabel = field_value.nodeLabel();
    new_field_value.integrationPoint = field_value.integrationPoint();
    if (new_field_value.integrationPoint != -1) { new_field_value.empty = false; }
    switch(field_value.type()) {
        case odb_Enum::SCALAR: new_field_value.type = "Scalar"; break;
        case odb_Enum::VECTOR: new_field_value.type = "Vector"; break;
        case odb_Enum::TENSOR_3D_FULL: new_field_value.type = "Tensor 3D Full"; break;
        case odb_Enum::TENSOR_3D_PLANAR: new_field_value.type = "Tensor 3D Planar"; break;
        case odb_Enum::TENSOR_3D_SURFACE: new_field_value.type = "Tensor 3D Surface"; break;
        case odb_Enum::TENSOR_2D_PLANAR: new_field_value.type = "Tensor 2D Planar"; break;
        case odb_Enum::TENSOR_2D_SURFACE: new_field_value.type = "Tensor 2D Surface"; break;
    }
    if (invariants.isMember(odb_Enum::MAGNITUDE)) {
        new_field_value.magnitude = field_value.magnitude();
        new_field_value.empty = false;
    } else { new_field_value.magnitude = 0; }
    if (invariants.isMember(odb_Enum::TRESCA)) {
        new_field_value.tresca = field_value.tresca();
        new_field_value.empty = false;
    } else { new_field_value.tresca = 0; }
    if (invariants.isMember(odb_Enum::PRESS)) {
        new_field_value.press = field_value.press();
        new_field_value.empty = false;
    } else { new_field_value.press = 0; }
    if (invariants.isMember(odb_Enum::INV3)) {
        new_field_value.inv3 = field_value.inv3();
        new_field_value.empty = false;
    } else { new_field_value.inv3 = 0; }
    if (invariants.isMember(odb_Enum::MAX_PRINCIPAL)) {
        new_field_value.maxPrincipal = field_value.maxPrincipal();
        new_field_value.empty = false;
    } else { new_field_value.maxPrincipal = 0; }
    if (invariants.isMember(odb_Enum::MID_PRINCIPAL)) {
        new_field_value.midPrincipal = field_value.midPrincipal();
        new_field_value.empty = false;
    } else { new_field_value.midPrincipal = 0; }
    if (invariants.isMember(odb_Enum::MIN_PRINCIPAL)) {
        new_field_value.minPrincipal = field_value.minPrincipal();
        new_field_value.empty = false;
    } else { new_field_value.minPrincipal = 0; }
    if (invariants.isMember(odb_Enum::MAX_INPLANE_PRINCIPAL)) {
        new_field_value.maxInPlanePrincipal = field_value.maxInPlanePrincipal();
        new_field_value.empty = false;
    } else { new_field_value.maxInPlanePrincipal = 0; }
    if (invariants.isMember(odb_Enum::MIN_INPLANE_PRINCIPAL)) {
        new_field_value.minInPlanePrincipal = field_value.minInPlanePrincipal();
        new_field_value.empty = false;
    } else { new_field_value.minInPlanePrincipal = 0; }
    if (invariants.isMember(odb_Enum::OUTOFPLANE_PRINCIPAL)) {
        new_field_value.outOfPlanePrincipal = field_value.outOfPlanePrincipal();
        new_field_value.empty = false;
    } else { new_field_value.outOfPlanePrincipal = 0; }
    new_field_value.sectionPoint.number =  to_string(field_value.sectionPoint().number());
    if (new_field_value.sectionPoint.number != "-1") { new_field_value.empty = false; }
    new_field_value.sectionPoint.description =  field_value.sectionPoint().description().CStr();
    return new_field_value;
}

field_bulk_type SpadeObject::process_field_bulk_data(const odb_FieldBulkData &field_bulk_data, const odb_SequenceInvariant& invariants, bool complex_data, Logging &log_file, CmdLineArguments &command_line_arguments) {
    field_bulk_type new_field_bulk_data;
    new_field_bulk_data.emptyFaces = true;
    switch(field_bulk_data.position()) {
        case odb_Enum::NODAL: new_field_bulk_data.position = "Nodal"; break;
        case odb_Enum::INTEGRATION_POINT: new_field_bulk_data.position = "Integration Point"; break;
        case odb_Enum::ELEMENT_NODAL: new_field_bulk_data.position = "Element Nodal"; break;
        case odb_Enum::ELEMENT_FACE: new_field_bulk_data.position = "Element Face"; break;
        case odb_Enum::CENTROID: new_field_bulk_data.position = "Centroid"; break;
    }
    new_field_bulk_data.instance = field_bulk_data.instance().name().CStr();
    new_field_bulk_data.length = field_bulk_data.length();
    new_field_bulk_data.width = field_bulk_data.width();

    float* data = 0;
    double* data_double = 0;
    float* conjugate_data = 0;
    double* conjugate_data_double = 0;
    float* local_coordinate_system = 0;
    double* local_coordinate_system_double = 0;


    if(field_bulk_data.precision() == odb_Enum::SINGLE_PRECISION) {
        new_field_bulk_data.precision = "Single Precision";
        data = field_bulk_data.data();
        conjugate_data = field_bulk_data.conjugateData();
        local_coordinate_system = field_bulk_data.localCoordSystem();
    } else {
        new_field_bulk_data.precision = "Double Precision";
        data_double = field_bulk_data.dataDouble();
        conjugate_data_double = field_bulk_data.conjugateDataDouble();
        local_coordinate_system_double = field_bulk_data.localCoordSystemDouble();
    }

    new_field_bulk_data.numberOfElements = 0;
    new_field_bulk_data.numberOfElements = field_bulk_data.numberOfElements();                
    new_field_bulk_data.valuesPerElement = 0;
    new_field_bulk_data.valuesPerElement = field_bulk_data.valuesPerElement();                
    int* element_labels = field_bulk_data.elementLabels();
    float* bulk_mises = field_bulk_data.mises();

    new_field_bulk_data.baseElementType = ""; // initializing to empty string

    if(new_field_bulk_data.numberOfElements && element_labels) { // If elements
        int number_of_integration_points = new_field_bulk_data.length/new_field_bulk_data.numberOfElements;
        int* integration_points = field_bulk_data.integrationPoints();
        odb_Enum::odb_ElementFaceEnum* faces = field_bulk_data.faces();
        new_field_bulk_data.orientationWidth = field_bulk_data.orientationWidth();
        new_field_bulk_data.baseElementType = field_bulk_data.baseElementType().CStr();


        odb_SequenceString component_labels = field_bulk_data.componentLabels();
        for (int i=0; i<field_bulk_data.componentLabels().size(); i++) {
            new_field_bulk_data.componentLabels.push_back(component_labels[i].CStr());
        }

        int current_position = 0;
        if(field_bulk_data.precision() == odb_Enum::SINGLE_PRECISION) {
            vector<vector <float>> current_local_coordinate_system_2D;
            vector<vector <float>> current_data_2D;
            vector<vector <float>> current_conjugate_data_2D;
            for (int element=0; element<new_field_bulk_data.numberOfElements; ++element) {
                vector<int> current_element_labels;
                vector<int> current_integration_points;
                vector<string> current_faces;
                vector<float> current_mises;
                vector<float> current_local_coordinate_system;
                vector<float> current_data;
                vector<float> current_conjugate_data;
                for (int integration_point=0; integration_point<number_of_integration_points; integration_point++, current_position++) {
                    current_element_labels.push_back(element_labels[current_position]);
                    if (integration_points) { current_integration_points.push_back(integration_points[current_position]); }
                    if (faces) { 
                        current_faces.push_back(this->faces_enum_strings[faces[current_position]]); 
                        new_field_bulk_data.emptyFaces = false;
                    }
                    if (local_coordinate_system) {
                        int current_pointer = current_position*new_field_bulk_data.orientationWidth;
                        for (int coordinate_point=0; coordinate_point<new_field_bulk_data.orientationWidth; ++coordinate_point) {
                            current_local_coordinate_system.push_back(local_coordinate_system[current_pointer++]);
                        }
                    }
                    int total_points = current_position*new_field_bulk_data.width;
                    for (int component=0; component<new_field_bulk_data.width; ++component) {
                        current_data.push_back(data[total_points++]);
//                        ostringstream float_string;
//                        float_string << std::scientific << current_data[current_data.size() - 1];
//                        log_file.logDebug("element: " + to_string(element + 1) + " integration point: " + to_string(integration_point + 1) + " component: " + to_string(component + 1) + " float data: " + float_string.str());
                    }
                    if (complex_data) {
                        total_points = current_position*field_bulk_data.width;
                        for (int component=0; component<field_bulk_data.width; ++component) {
                            current_conjugate_data.push_back(conjugate_data[total_points++]);
                        }
                    }
                    if (invariants.isMember(odb_Enum::MISES)) {
                        current_mises.push_back(bulk_mises[current_position]);
                    }
                }
                new_field_bulk_data.elementLabels.push_back(current_element_labels);
                new_field_bulk_data.integrationPoints.push_back(current_integration_points);
                new_field_bulk_data.faces.push_back(current_faces);
                new_field_bulk_data.mises.push_back(current_mises);
                current_local_coordinate_system_2D.push_back(current_local_coordinate_system);
                current_data_2D.push_back(current_data);
                current_conjugate_data_2D.push_back(current_conjugate_data);
            }
            new_field_bulk_data.element.data.push_back(current_data_2D);
            new_field_bulk_data.element.localCoordSystem.push_back(current_local_coordinate_system_2D);
            new_field_bulk_data.element.conjugateData.push_back(current_conjugate_data_2D);
        } else {
            vector<vector <double>> current_local_coordinate_system_2D;
            vector<vector <double>> current_data_2D;
            vector<vector <double>> current_conjugate_data_2D;
            for (int element=0; element<new_field_bulk_data.numberOfElements; ++element) {
                vector<int> current_element_labels;
                vector<int> current_integration_points;
                vector<string> current_faces;
                vector<float> current_mises;
                vector<double> current_local_coordinate_system;
                vector<double> current_data;
                vector<double> current_conjugate_data;
                for (int integration_point=0; integration_point<number_of_integration_points; integration_point++, current_position++) {
                    current_element_labels.push_back(element_labels[current_position]);
                    if (integration_points) { current_integration_points.push_back(integration_points[current_position]); }
                    if (faces) { 
                        current_faces.push_back(this->faces_enum_strings[faces[current_position]]); 
                        new_field_bulk_data.emptyFaces = false;
                    }
                    if (local_coordinate_system) {
                        int current_pointer = current_position*new_field_bulk_data.orientationWidth;
                        for (int coordinate_point=0; coordinate_point<new_field_bulk_data.orientationWidth; ++coordinate_point) {
                            current_local_coordinate_system.push_back(local_coordinate_system[current_pointer++]);
                        }
                    }
                    int total_points = current_position*new_field_bulk_data.width;
                    for (int component=0; component<new_field_bulk_data.width; ++component) {
                        current_data.push_back(data[total_points++]);
//                        ostringstream double_string;
//                        double_string << std::scientific << current_data[current_data.size() - 1];
//                        log_file.logDebug("element: " + to_string(element + 1) + " integration point: " + to_string(integration_point + 1) + " component: " + to_string(component + 1) + " double data: " + double_string.str());
                    }
                    if (complex_data) {
                        total_points = current_position*field_bulk_data.width;
                        for (int component=0; component<field_bulk_data.width; ++component) {
                            current_conjugate_data.push_back(conjugate_data[total_points++]);
                        }
                    }
                    if (invariants.isMember(odb_Enum::MISES)) {
                        current_mises.push_back(bulk_mises[current_position]);
                    }
                }
                new_field_bulk_data.elementLabels.push_back(current_element_labels);
                new_field_bulk_data.integrationPoints.push_back(current_integration_points);
                new_field_bulk_data.faces.push_back(current_faces);
                new_field_bulk_data.mises.push_back(current_mises);
                current_local_coordinate_system_2D.push_back(current_local_coordinate_system);
                current_data_2D.push_back(current_data);
                current_conjugate_data_2D.push_back(current_conjugate_data);
            }
            new_field_bulk_data.element.dataDouble.push_back(current_data_2D);
            new_field_bulk_data.element.localCoordSystemDouble.push_back(current_local_coordinate_system_2D);
            new_field_bulk_data.element.conjugateDataDouble.push_back(current_conjugate_data_2D);
        }
    } else {  // Nodes
        int* node_labels = field_bulk_data.nodeLabels();	
        if(field_bulk_data.precision() == odb_Enum::SINGLE_PRECISION) {
            for (int node_count=0; node_count<new_field_bulk_data.length; ++node_count) {
                vector<float> current_data;
                int total_points = node_count*new_field_bulk_data.width;
                for (int component=0; component<new_field_bulk_data.width; ++component) {
                    current_data.push_back(data[total_points++]);
                }
                new_field_bulk_data.nodeLabels.push_back(node_labels[node_count]);
                new_field_bulk_data.data.push_back(current_data);
            }
            if (complex_data) {
                for (int node_count=0; node_count<new_field_bulk_data.length; ++node_count) {
                    vector<float> current_conjugate_data;
                    int total_points = node_count*new_field_bulk_data.width;
                    for (int component=0; component<new_field_bulk_data.width; ++component) {
                        current_conjugate_data.push_back(conjugate_data[total_points++]);
                    }
                    new_field_bulk_data.conjugateData.push_back(current_conjugate_data);
                }
            }      
        } else {
            for (int node_count=0; node_count<new_field_bulk_data.length; ++node_count) {
                vector<double> current_data_double;
                vector<int> current_node_labels;
                current_node_labels.push_back(node_labels[node_count]);
                int total_points = node_count*new_field_bulk_data.width;
                for (int component=0; component<new_field_bulk_data.width; ++component) {
                    current_data_double.push_back(data_double[total_points++]);
                }
                new_field_bulk_data.dataDouble.push_back(current_data_double);
            }             
            if (complex_data) {
                for (int node_count=0; node_count<new_field_bulk_data.length; ++node_count) {
                    vector<double> current_conjugate_data_double;
                    int total_points = node_count*new_field_bulk_data.width;
                    for (int component=0; component<new_field_bulk_data.width; ++component) {
                        current_conjugate_data_double.push_back(conjugate_data_double[total_points++]);
                    }
                    new_field_bulk_data.conjugateDataDouble.push_back(current_conjugate_data_double);
                }
            }
        }
    }
    return new_field_bulk_data;
}

field_output_type SpadeObject::process_field_output (const odb_FieldOutput &field_output, Logging &log_file, CmdLineArguments &command_line_arguments) {
    field_output_type new_field_output;
    new_field_output.name = field_output.name().CStr();
    new_field_output.description = field_output.description().CStr();
    new_field_output.dim = field_output.dim();
    new_field_output.dim2 = field_output.dim2();
    // TODO: Maybe reach out to 3DS to determine if they plan to implement isEngineeringTensor() function
//    new_field_output.isEngineeringTensor = (field_output.isEngineeringTensor()) ? "true" : "false";
    odb_SequenceFieldLocation field_locations = field_output.locations();
    for (int i=0; i<field_locations.size(); i++) {
        odb_FieldLocation field_location = field_locations.constGet(i);
        field_location_type new_field_location;
        switch(field_location.position()) {
            case odb_Enum::NODAL: new_field_location.position = "Nodal"; break;
            case odb_Enum::INTEGRATION_POINT: new_field_location.position = "Integration Point"; break;
            case odb_Enum::ELEMENT_NODAL: new_field_location.position = "Element Nodal"; break;
            case odb_Enum::ELEMENT_FACE: new_field_location.position = "Element Face"; break;
            case odb_Enum::CENTROID: new_field_location.position = "Centroid"; break;
        }
        for (int i=0; i<field_location.sectionPoint().size(); i++) {
            section_point_type point;
            point.number = to_string(field_location.sectionPoint(i).number());
            point.description = field_location.sectionPoint(i).description().CStr();
            new_field_location.sectionPoint.push_back(point);
        }
        new_field_output.locations.push_back(new_field_location);
    }
    odb_SequenceString available_components = field_output.componentLabels();
    for (int i=0; i<available_components.size(); i++) {
        new_field_output.componentLabels.push_back(available_components[i].CStr());
    }
    for (int i=0; i<field_output.validInvariants().size(); i++) {
        string invariant;
        switch(field_output.validInvariants().constGet(i)) {
            case odb_Enum::MAGNITUDE: invariant = "Magnitude"; break;
            case odb_Enum::MISES: invariant = "Mises"; break;
            case odb_Enum::TRESCA: invariant = "Tresca"; break;
            case odb_Enum::PRESS: invariant = "Press"; break;
            case odb_Enum::INV3: invariant = "Inv3"; break;
            case odb_Enum::MAX_PRINCIPAL: invariant = "Max Principal"; break;
            case odb_Enum::MID_PRINCIPAL: invariant = "Mid Principal"; break;
            case odb_Enum::MIN_PRINCIPAL: invariant = "Min Principal"; break;
            case odb_Enum::MAX_INPLANE_PRINCIPAL: invariant = "Max Inplane Principal"; break;
            case odb_Enum::MIN_INPLANE_PRINCIPAL: invariant = "Min Inplane Principal"; break;
            case odb_Enum::OUTOFPLANE_PRINCIPAL: invariant = "Out of Plane Principal"; break;
        }
        new_field_output.validInvariants.push_back(invariant.c_str());
    }
    odb_SequenceFieldValue field_values = field_output.values();
    log_file.logVerbose("Reading field values.");
    new_field_output.node_values_empty = true;
    new_field_output.element_values_empty = true;
    if (field_output.validInvariants().size() > 0) {
        for (int i=0; i<field_values.size(); i++) {
            const odb_FieldValue& field_value = field_values.constGet(i);
            field_value_type new_field_value = process_field_values(field_value, field_output.validInvariants(), log_file, command_line_arguments);
            if (!new_field_value.empty) {
                if (new_field_value.elementLabel != -1) { 
                    new_field_output.elementValues[new_field_value.elementLabel] = new_field_value; 
                    new_field_output.element_values_empty = false;
                }
                if (new_field_value.nodeLabel != -1) { 
                    new_field_output.nodeValues[new_field_value.nodeLabel] = new_field_value;
                    new_field_output.node_values_empty = false;
                }
            }
//            new_field_output.values.push_back(process_field_values(field_value, field_output.validInvariants(), log_file, command_line_arguments));
        }
    }
    new_field_output.max_length = 0;
    new_field_output.max_width = 0;
    const odb_SequenceFieldBulkData& field_bulk_values = field_output.bulkDataBlocks();	
    new_field_output.isComplex = field_output.isComplex();
    log_file.logVerbose("Reading field bulk data.");
    for (int i=0; i<field_bulk_values.size(); i++) {
        const odb_FieldBulkData& field_bulk_value = field_bulk_values[i];
        new_field_output.dataValues.push_back(process_field_bulk_data(field_bulk_value, field_output.validInvariants(), field_output.isComplex(), log_file, command_line_arguments));
        if (new_field_output.dataValues[i].width > new_field_output.max_width) {  new_field_output.max_width = new_field_output.dataValues[i].width; }
        if (new_field_output.dataValues[i].length > new_field_output.max_length) {  new_field_output.max_length = new_field_output.dataValues[i].length; }
    }
    return new_field_output;
}

frame_type SpadeObject::process_frame (const odb_Frame &frame, Logging &log_file, CmdLineArguments &command_line_arguments) {
    frame_type new_frame;
    new_frame.description = frame.description().CStr();
    new_frame.loadCase = frame.loadCase().name().CStr();
    switch(frame.domain()) {
        case odb_Enum::TIME: new_frame.domain = "Time"; break;
        case odb_Enum::FREQUENCY: new_frame.domain = "Frequency"; break;
        case odb_Enum::MODAL: new_frame.domain = "Modal"; break;
    }
    new_frame.incrementNumber = frame.incrementNumber();
    new_frame.cyclicModeNumber = frame.cyclicModeNumber();
    new_frame.mode = frame.mode();
    new_frame.frameValue = frame.frameValue();
    new_frame.frequency = frame.frequency();

    const odb_FieldOutputRepository& field_outputs = frame.fieldOutputs();
    odb_FieldOutputRepositoryIT field_outputs_iterator(field_outputs);
    log_file.logVerbose("Reading field output.");
    new_frame.max_length = 0;
    new_frame.max_width = 0;
    int field_output_count = 0;
    for (field_outputs_iterator.first(); !field_outputs_iterator.isDone(); field_outputs_iterator.next()) {
        const odb_FieldOutput& field = field_outputs[field_outputs_iterator.currentKey()]; 
        new_frame.fieldOutputs.push_back(process_field_output(field, log_file, command_line_arguments));
        if (new_frame.fieldOutputs[field_output_count].max_width > new_frame.max_width) {  new_frame.max_width = new_frame.fieldOutputs[field_output_count].max_width; }
        if (new_frame.fieldOutputs[field_output_count].max_length > new_frame.max_length) {  new_frame.max_length = new_frame.fieldOutputs[field_output_count].max_length; }
        field_output_count++;
    }
    return new_frame;
}

history_point_type SpadeObject::process_history_point (const odb_HistoryPoint history_point, Logging &log_file) {
    history_point_type new_history_point;
    try { 
        new_history_point.element = process_element(history_point.element(), log_file);
        new_history_point.hasElement = true;
    } catch(odb_BaseException& exc) { new_history_point.hasElement = false; }
    new_history_point.node = process_node(history_point.node(), log_file);
    if (history_point.node().label() < 0) {
        new_history_point.hasNode = false;
    } else {
        new_history_point.hasNode = true;
    }
    new_history_point.ipNumber = history_point.ipNumber();
    new_history_point.sectionPoint.number = to_string(history_point.sectionPoint().number());
    new_history_point.sectionPoint.description = history_point.sectionPoint().description().CStr();
    new_history_point.region = process_set(history_point.region(), log_file);
    new_history_point.instanceName = history_point.instance().name().CStr();
    new_history_point.assemblyName = history_point.assembly().name().CStr();
    switch(history_point.face()) {
        case odb_Enum::FACE_UNKNOWN: new_history_point.face = "Face Unknown"; break;
        case odb_Enum::FACE1: new_history_point.face = "Face 1"; break;
        case odb_Enum::FACE2: new_history_point.face = "Face 2"; break;
        case odb_Enum::FACE3: new_history_point.face = "Face 3"; break;
        case odb_Enum::FACE4: new_history_point.face = "Face 4"; break;
        case odb_Enum::FACE5: new_history_point.face = "Face 5"; break;
        case odb_Enum::FACE6: new_history_point.face = "Face 6"; break;
        case odb_Enum::SIDE1: new_history_point.face = "Side 1"; break;
        case odb_Enum::SIDE2: new_history_point.face = "Side 2"; break;
        case odb_Enum::END1: new_history_point.face = "End 1"; break;
        case odb_Enum::END2: new_history_point.face = "End 2"; break;
        case odb_Enum::END3: new_history_point.face = "End 3"; break;
    }
    switch(history_point.position()) {
        case odb_Enum::NODAL: new_history_point.position = "Nodal"; break;
        case odb_Enum::ELEMENT_NODAL: new_history_point.position = "Element Nodal"; break;
        case odb_Enum::INTEGRATION_POINT: new_history_point.position = "Integration Point"; break;
        case odb_Enum::ELEMENT_FACE: new_history_point.position = "Element Face"; break;
        case odb_Enum::ELEMENT_FACE_INTEGRATION_POINT: new_history_point.position = "Element Face Integration Point"; break;
        case odb_Enum::WHOLE_ELEMENT: new_history_point.position = "Whole Element"; break;
        case odb_Enum::WHOLE_REGION: new_history_point.position = "Whole Region"; break;
        case odb_Enum::WHOLE_PART_INSTANCE: new_history_point.position = "Whole Part Instance"; break;
        case odb_Enum::WHOLE_MODEL: new_history_point.position = "Whole Model"; break;
    }

    return new_history_point;
}

history_output_type SpadeObject::process_history_output (const odb_HistoryOutput &history_output, Logging &log_file, CmdLineArguments &command_line_arguments) {
    history_output_type new_history_output;
    new_history_output.name = history_output.name().CStr();
    new_history_output.description = history_output.description().CStr();
    switch(history_output.type()) {
        case odb_Enum::SCALAR: new_history_output.type = "Scalar"; break;
    }

    const odb_SequenceSequenceFloat& data = history_output.data();
    new_history_output.max_column_size = 0;
    for (int i=0; i<data.size(); i++) {
        odb_SequenceFloat data_dimension1 = data.constGet(i);
        if (data_dimension1.size() > new_history_output.max_column_size) { new_history_output.max_column_size = data_dimension1.size(); }
        vector<float> dimension1;
        for (int j=0; j<data_dimension1.size(); j++) {
            dimension1.push_back(data_dimension1.constGet(j));
        }
        new_history_output.data.push_back(dimension1);
    }

    const odb_SequenceSequenceFloat& conjugate_data = history_output.conjugateData();
    new_history_output.max_column_size_conjugate = 0;
    for (int i=0; i<conjugate_data.size(); i++) {
        odb_SequenceFloat conjugate_data_dimension1 = conjugate_data.constGet(i);
        if (conjugate_data_dimension1.size() > new_history_output.max_column_size_conjugate) { new_history_output.max_column_size_conjugate = conjugate_data_dimension1.size(); }
        vector<float> dimension1;
        for (int j=0; j<conjugate_data_dimension1.size(); j++) {
            dimension1.push_back(conjugate_data_dimension1.constGet(j));
        }
        new_history_output.conjugateData.push_back(dimension1);
    }

    return new_history_output;

}

history_region_type SpadeObject::process_history_region (const odb_HistoryRegion &history_region, Logging &log_file, CmdLineArguments &command_line_arguments) {
    history_region_type new_history_region;
    new_history_region.name = history_region.name().CStr();
    new_history_region.description = history_region.description().CStr();
    switch(history_region.position()) {
        case odb_Enum::NODAL: new_history_region.position = "Nodal"; break;
        case odb_Enum::INTEGRATION_POINT: new_history_region.position = "Integration Point"; break;
        case odb_Enum::WHOLE_ELEMENT: new_history_region.position = "Whole Element"; break;
        case odb_Enum::WHOLE_REGION: new_history_region.position = "Whole Region"; break;
        case odb_Enum::WHOLE_MODEL: new_history_region.position = "Whole Model"; break;
    }
    new_history_region.loadCase = history_region.loadCase().name().CStr();
    new_history_region.point = process_history_point(history_region.historyPoint(), log_file);
    const odb_HistoryOutputRepository& history_outputs = history_region.historyOutputs();
    odb_HistoryOutputRepositoryIT history_outputs_iterator (history_outputs);
    log_file.logVerbose("Reading history output.");
    for (history_outputs_iterator.first(); !history_outputs_iterator.isDone(); history_outputs_iterator.next()) {
        odb_HistoryOutput history_output = history_outputs_iterator.currentValue();
        new_history_region.historyOutputs.push_back( process_history_output(history_output, log_file, command_line_arguments));
    }


    return new_history_region;
}

void SpadeObject::process_step(const odb_Step &step, odb_Odb &odb, Logging &log_file, CmdLineArguments &command_line_arguments) {
    step_type new_step;
    new_step.name = step.name().CStr();
    if ((command_line_arguments["step"] != "all") && (command_line_arguments["step"] != new_step.name)) {
        return;
    }
    new_step.description = step.description().CStr();
    switch(step.domain()) {
        case odb_Enum::TIME: new_step.domain = "Time"; break;
        case odb_Enum::FREQUENCY: new_step.domain = "Frequency"; break;
        case odb_Enum::MODAL: new_step.domain = "Modal"; break;
        case odb_Enum::ARC_LENGTH: new_step.domain = "Arc length"; break;
    }
    new_step.previousStepName = step.previousStepName().CStr();
    new_step.procedure = step.procedure().CStr();
    new_step.nlgeom = (step.nlgeom()) ? "yes" : "no";
    new_step.number = step.number();
    new_step.timePeriod = step.timePeriod();
    new_step.totalTime = step.totalTime();
    new_step.mass = step.mass();
    new_step.acousticMass = step.acousticMass();
    odb_SequenceDouble mass_center = step.massCenter();
    for (int i=0; i<mass_center.size(); i++) { new_step.massCenter.push_back(mass_center[i]); }
    odb_SequenceDouble acoustic_mass_center = step.acousticMassCenter();
    for (int i=0; i<acoustic_mass_center.size(); i++) { new_step.acousticMassCenter.push_back(acoustic_mass_center[i]); }
    odb_SequenceDouble inertia_about_center = step.inertiaAboutCenter();
    for (int i=0; i<inertia_about_center.size(); i++) { new_step.inertiaAboutCenter[i] = inertia_about_center[i]; }
    odb_SequenceDouble inertia_about_origin = step.inertiaAboutOrigin();
    for (int i=0; i<inertia_about_origin.size(); i++) { new_step.inertiaAboutOrigin[i] = inertia_about_origin[i]; }
    odb_LoadCaseRepository load_cases = step.loadCases();
    for (int i=0; i<load_cases.size(); i++) { new_step.loadCases.push_back(load_cases[i].name().CStr()); }
    const odb_SequenceFrame& frames = step.frames();
    log_file.logVerbose("Reading frames.");
    for (int f=0; f<frames.size(); f++) {
        const odb_Frame& frame = frames.constGet(f);
        new_step.frames.push_back(process_frame(frame, log_file, command_line_arguments));
    }
    const odb_HistoryRegionRepository& history_regions = step.historyRegions();
    odb_HistoryRegionRepositoryIT history_region_iterator (history_regions);
    log_file.logVerbose("Reading history regions.");
    for (history_region_iterator.first(); !history_region_iterator.isDone(); history_region_iterator.next()) 
    {
        const odb_HistoryRegion& history_region = history_region_iterator.currentValue();
        new_step.historyRegions.push_back(process_history_region(history_region, log_file, command_line_arguments));
    }
    // TODO: Write code to handle command line arguments that limit how much history or field output data is written
    this->steps.push_back(new_step);
 cout << this->steps[0].frames[1].fieldOutputs[2].dataValues[0].data[0] << endl;
 cout << this->steps[0].frames[2].fieldOutputs[2].dataValues[0].data[0] << endl;
}

void SpadeObject::write_h5 (CmdLineArguments &command_line_arguments, Logging &log_file) {
// Write out data to hdf5 file

    // Open file for writing
    std::ifstream hdf5File (command_line_arguments["output-file"].c_str());
    log_file.logDebug("Creating hdf5 file " + command_line_arguments["output-file"]);
    const H5std_string FILE_NAME(command_line_arguments["output-file"]);

    H5::Exception::dontPrint();
    H5::H5File* h5_file_pointer = 0;
    try {
        h5_file_pointer = new H5::H5File(FILE_NAME, H5F_ACC_TRUNC);
    } catch(const H5::FileIException&) {
        cerr << "Issue opening file: " << command_line_arguments["output-file"] << "\n";
        perror(""); throw std::exception(); std::terminate(); //print error, throw exception and terminate
    }
    H5::H5File h5_file = *h5_file_pointer;

    log_file.logVerbose("Writing top level data to odb group.");
    H5::Group odb_group = h5_file.createGroup(string("/odb").c_str());
    write_string_dataset(odb_group, "name", this->name);
    write_string_dataset(odb_group, "analysisTitle", this->analysisTitle);
    write_string_dataset(odb_group, "description", this->description);
    write_string_dataset(odb_group, "path", this->path);
    write_string_dataset(odb_group, "isReadOnly", this->isReadOnly);

    log_file.logVerbose("Writing jobData.");
    H5::Group job_data_group = h5_file.createGroup(string("/odb/jobData").c_str());
    write_string_dataset(job_data_group, "analysisCode", this->job_data.analysisCode);
    write_string_dataset(job_data_group, "creationTime", this->job_data.creationTime);
    write_string_dataset(job_data_group, "machineName", this->job_data.machineName);
    write_string_dataset(job_data_group, "modificationTime", this->job_data.modificationTime);
    write_string_dataset(job_data_group, "name", this->job_data.name);
    write_string_dataset(job_data_group, "precision", this->job_data.precision);
    write_string_vector_dataset(job_data_group, "productAddOns", this->job_data.productAddOns);
    write_string_dataset(job_data_group, "version", this->job_data.version);

    log_file.logVerbose("Writing sector definition.");
    H5::Group sector_definition_group = h5_file.createGroup(string("/odb/sectorDefinition").c_str());
    write_integer_dataset(sector_definition_group, "numSectors", this->sector_definition.numSectors);
    H5::Group symmetry_axis_group = h5_file.createGroup(string("/odb/sectorDefinition/symmetryAxis").c_str());
    write_string_dataset(symmetry_axis_group, "StartPoint", this->sector_definition.start_point);
    write_string_dataset(symmetry_axis_group, "EndPoint", this->sector_definition.end_point);

    log_file.logVerbose("Writing section categories.");
    H5::Group section_categories_group = h5_file.createGroup(string("/odb/sectionCategories").c_str());
    for (int i=0; i<this->section_categories.size(); i++) {
        string category_group_name = "/odb/sectionCategories/" + this->section_categories[i].name;
        H5::Group section_category_group = h5_file.createGroup(category_group_name.c_str());
        write_section_category(h5_file, section_category_group, category_group_name, this->section_categories[i]);
    }

    log_file.logVerbose("Writing user data.");
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

    log_file.logVerbose("Writing constraints data.");
    H5::Group contraints_group = h5_file.createGroup(string("/odb/constraints").c_str());
    write_constraints(h5_file, "odb/constraints");
    log_file.logVerbose("Writing interactions data.");
    write_interactions(h5_file, "odb");
    H5::Group parts_group = h5_file.createGroup(string("odb/parts").c_str());
    log_file.logVerbose("Writing parts data.");
    write_parts(h5_file, "odb/parts");
    log_file.logVerbose("Writing assembly data.");
    write_assembly(h5_file, "odb/rootAssembly");
    log_file.logVerbose("Writing steps data.");
    write_steps(h5_file, log_file, "odb");

    h5_file.close();  // Close the hdf5 file
    log_file.logVerbose("Closing hdf5 file.");
}

void SpadeObject::write_parts(H5::H5File &h5_file, const string &group_name) {
    for (auto part : this->parts) {
        string part_group_name = group_name + "/" + part.name;
        H5::Group part_group = h5_file.createGroup(part_group_name.c_str());
        write_string_dataset(part_group, "embeddedSpace", part.embeddedSpace);
        write_nodes(h5_file, part_group_name, part.nodes);
        write_elements(h5_file, part_group_name, part.elements);
        write_sets(h5_file, part_group_name + "/nodeSets", part.nodeSets);
        write_sets(h5_file, part_group_name + "/elementSets", part.elementSets);
        write_sets(h5_file, part_group_name + "/surfaces", part.surfaces);
    }
}

void SpadeObject::write_assembly(H5::H5File &h5_file, const string &group_name) {
    string root_assembly_group_name = "/odb/rootAssembly " + this->root_assembly.name;
    H5::Group root_assembly_group = h5_file.createGroup(root_assembly_group_name.c_str());
    write_instances(h5_file, root_assembly_group_name);
    write_string_dataset(root_assembly_group, "embeddedSpace", this->root_assembly.embeddedSpace);
    write_nodes(h5_file, root_assembly_group_name, this->root_assembly.nodes);
    write_elements(h5_file, root_assembly_group_name, this->root_assembly.elements);
    write_sets(h5_file, root_assembly_group_name + "/nodeSets", this->root_assembly.nodeSets);
    write_sets(h5_file, root_assembly_group_name + "/elementSets", this->root_assembly.elementSets);
    write_sets(h5_file, root_assembly_group_name + "/surfaces", this->root_assembly.surfaces);
    H5::Group connector_orientations_group = h5_file.createGroup((root_assembly_group_name + "/connectorOrientations").c_str());
    for (int i=0; i<this->root_assembly.connectorOrientations.size(); i++) {
        string connector_orientation_group_name = root_assembly_group_name + "/connectorOrientations/" + to_string(i);
        H5::Group connector_orientation_group = h5_file.createGroup(connector_orientation_group_name.c_str());
        write_set(h5_file, connector_orientation_group_name, this->root_assembly.connectorOrientations[i].region);
        write_string_dataset(connector_orientation_group, "orient2sameAs1", this->root_assembly.connectorOrientations[i].orient2sameAs1);
        write_float_dataset(connector_orientation_group, "angle1", this->root_assembly.connectorOrientations[i].angle1);
        write_float_dataset(connector_orientation_group, "angle2", this->root_assembly.connectorOrientations[i].angle2);
        write_datum_csys(h5_file, connector_orientation_group_name, this->root_assembly.connectorOrientations[i].localCsys1);
        write_datum_csys(h5_file, connector_orientation_group_name, this->root_assembly.connectorOrientations[i].localCsys2);
        write_string_dataset(connector_orientation_group, "axis1", this->root_assembly.connectorOrientations[i].axis1);
        write_string_dataset(connector_orientation_group, "axis2", this->root_assembly.connectorOrientations[i].axis2);
    }
}

void SpadeObject::write_field_value(H5::H5File &h5_file, const string &group_name, field_value_type &field_value) {
    H5::Group value_group = h5_file.createGroup(group_name.c_str());
    if (field_value.elementLabel != -1) {
        write_integer_dataset(value_group, "elementLabel", field_value.elementLabel);
    }
    if (field_value.nodeLabel != -1) {
        write_integer_dataset(value_group, "nodeLabel", field_value.nodeLabel);
    }
    if (field_value.integrationPoint != -1) {
        write_integer_dataset(value_group, "integrationPoint", field_value.integrationPoint);
    }
    if (field_value.magnitude != 0) {
        write_float_dataset(value_group, "magnitude", field_value.magnitude);
    }
    if (field_value.tresca != 0) {
        write_float_dataset(value_group, "tresca", field_value.tresca);
    }
    if (field_value.press != 0) {
        write_float_dataset(value_group, "press", field_value.press);
    }
    if (field_value.inv3 != 0) {
        write_float_dataset(value_group, "inv3", field_value.inv3);
    }
    if (field_value.maxPrincipal != 0) {
        write_float_dataset(value_group, "maxPrincipal", field_value.maxPrincipal);
    }
    if (field_value.midPrincipal != 0) {
        write_float_dataset(value_group, "midPrincipal", field_value.midPrincipal);
    }
    if (field_value.minPrincipal != 0) {
        write_float_dataset(value_group, "minPrincipal", field_value.minPrincipal);
    }
    if (field_value.maxInPlanePrincipal != 0) {
        write_float_dataset(value_group, "maxInPlanePrincipal", field_value.maxInPlanePrincipal);
    }
    if (field_value.minInPlanePrincipal != 0) {
        write_float_dataset(value_group, "minInPlanePrincipal", field_value.minInPlanePrincipal);
    }
    if (field_value.outOfPlanePrincipal != 0) {
        write_float_dataset(value_group, "outOfPlanePrincipal", field_value.outOfPlanePrincipal);
    }
    if (field_value.sectionPoint.number != "-1") {
        H5::Group section_point_group = h5_file.createGroup((group_name + "/sectionPoint").c_str());
        write_string_dataset(section_point_group, "number", field_value.sectionPoint.number);
        write_string_dataset(section_point_group, "description", field_value.sectionPoint.description);
    }
    write_string_dataset(value_group, "type", field_value.type);
}

void SpadeObject::write_field_bulk_data(H5::H5File &h5_file, Logging &log_file, const string &group_name, field_bulk_type &field_bulk_data, bool complex_data) {
    H5::Group bulk_group;
    /*
    H5::Exception::dontPrint();
    try {
        bulk_group = h5_file.openGroup(group_name.c_str());
    } catch (...) {
        bulk_group = h5_file.createGroup(group_name.c_str());
    }
    */
    bulk_group = h5_file.createGroup(group_name.c_str());
    write_string_dataset(bulk_group, "position", field_bulk_data.position);
    write_string_dataset(bulk_group, "instance", field_bulk_data.instance);
//    write_string_dataset(bulk_group, "precision", field_bulk_data.precision);
    if(field_bulk_data.numberOfElements && field_bulk_data.elementLabels) { // If elements
        bool empty_faces = true;
        int number_of_integration_points = field_bulk_data.length/field_bulk_data.numberOfElements;
        int element_labels[field_bulk_data.numberOfElements][number_of_integration_points];
        int integration_points[field_bulk_data.numberOfElements][number_of_integration_points];
        float mises[field_bulk_data.numberOfElements][number_of_integration_points];
        string faces[field_bulk_data.numberOfElements][number_of_integration_points];
        if (field_bulk_data.baseElementType != "") {
            write_string_dataset(bulk_group, "baseElementType", field_bulk_data.baseElementType);
        }
        write_integer_dataset(bulk_group, "orientationWidth", field_bulk_data.orientationWidth);
        if (field_bulk_data.numberOfElements != 0) {
            write_integer_dataset(bulk_group, "numberOfElements", field_bulk_data.numberOfElements);
        }
        if (field_bulk_data.valuesPerElement != 0) {
            write_integer_dataset(bulk_group, "valuesPerElement", field_bulk_data.valuesPerElement);
        }
        write_string_vector_dataset(bulk_group, "componentLabels", field_bulk_data.componentLabels);
        /*
        int current_position = 0;
        if(field_bulk_data.precision == "Single Precision") {
            float local_coordinate_system[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.orientationWidth];
            float data[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.width];
            float conjugate_data[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.width];
            for (int element=0; element<field_bulk_data.numberOfElements; ++element) {
                for (int integration_point=0; integration_point<number_of_integration_points; integration_point++, current_position++) {
                    element_labels[element][integration_point] = field_bulk_data.elementLabels[current_position];
                    if (field_bulk_data.integrationPoints) { integration_points[element][integration_point] = field_bulk_data.integrationPoints[current_position]; }
                    if (field_bulk_data.faces) { 
                        faces[element][integration_point] = faces_enum_strings[field_bulk_data.faces[current_position]]; 
                        empty_faces = false;
                    }
                    if (field_bulk_data.localCoordSystem) {
                        int current_pointer = current_position*field_bulk_data.orientationWidth;
                        for (int coordinate_point=0; coordinate_point<field_bulk_data.orientationWidth; ++coordinate_point) {
                            local_coordinate_system[element][integration_point][coordinate_point] = field_bulk_data.localCoordSystem[current_pointer++];
                        }
                    }
                    int total_points = current_position*field_bulk_data.width;
                    for (int component=0; component<field_bulk_data.width; ++component) {
             cout << "total_points " << total_points << endl;
                        data[element][integration_point][component] = field_bulk_data.data[total_points++];
//                        ostringstream float_string;
//                        float_string << std::scientific << data[element][integration_point][component];
//                        log_file.logDebug("element: " + to_string(element + 1) + " integration point: " + to_string(integration_point + 1) + " component: " + to_string(component + 1) + " float data: " + float_string.str());
                    }
                    if (complex_data) {
                        total_points = current_position*field_bulk_data.width;
                        for (int component=0; component<field_bulk_data.width; ++component) {
                            conjugate_data[element][integration_point][component] = field_bulk_data.conjugateData[total_points++];
                        }
                    }
                    if (!field_bulk_data.emptyMises) {
                        mises[element][integration_point] = field_bulk_data.mises[current_position];
                    }
                }
            }
            // Need to check if this data is empty or not
            hsize_t dimensions[] = {field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements};
            H5::DataSpace dataspace(3, dimensions);  // three dimensional data
            H5::DataSet dataset = bulk_group.createDataSet("data", H5::PredType::NATIVE_FLOAT, dataspace);
            dataset.write(data, H5::PredType::NATIVE_FLOAT);
            dataset.close();
            dataspace.close();
        */

            /*
            // Need to check if this data is empty or not
            hsize_t conjugate_dimensions[] = {field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements};
            H5::DataSpace conjugate_dataspace(3, conjugate_dimensions);  // three dimensional data
            H5::DataSet conjugate_dataset = bulk_group.createDataSet("conjugateData", H5::PredType::NATIVE_FLOAT, conjugate_dataspace);
            conjugate_dataset.write(conjugate_data, H5::PredType::NATIVE_FLOAT);
            conjugate_dataset.close();
            conjugate_dataspace.close();

            hsize_t coord_dimensions[] = {field_bulk_data.orientationWidth, number_of_integration_points, field_bulk_data.numberOfElements};
            H5::DataSpace coord_dataspace(3, coord_dimensions);  // three dimensional data
            H5::DataSet coord_dataset = bulk_group.createDataSet("data", H5::PredType::NATIVE_FLOAT, coord_dataspace);
            coord_dataset.write(local_coordinate_system, H5::PredType::NATIVE_FLOAT);
            coord_dataset.close();
            coord_dataspace.close();
            */

            /*
            write_float_3D_data(bulk_group, "data", field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements, *data);
            write_float_3D_data(bulk_group, "conjugateData", field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements, *conjugate_data);
            write_float_3D_data(bulk_group, "localCoordSystem", field_bulk_data.orientationWidth, number_of_integration_points, field_bulk_data.numberOfElements, *local_coordinate_system);
            */
           /*
        } else {  // Double precision
            double local_coordinate_system_double[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.orientationWidth];
            double data_double[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.width];
            double conjugate_data_double[field_bulk_data.numberOfElements][number_of_integration_points][field_bulk_data.width];
            for (int element=0; element<field_bulk_data.numberOfElements; ++element) {
                for (int integration_point=0; integration_point<number_of_integration_points; integration_point++, current_position++) {
                    element_labels[element][integration_point] = field_bulk_data.elementLabels[current_position];
                    if (field_bulk_data.integrationPoints) { integration_points[element][integration_point] = field_bulk_data.integrationPoints[current_position]; }
                    if (field_bulk_data.faces) { 
                        faces[element][integration_point] = faces_enum_strings[field_bulk_data.faces[current_position]]; 
                        empty_faces = false;
                    }
                    if (field_bulk_data.localCoordSystemDouble) {
                        int current_pointer = current_position*field_bulk_data.orientationWidth;
                        for (int coordinate_point=0; coordinate_point<field_bulk_data.orientationWidth; ++coordinate_point) {
                            local_coordinate_system_double[element][integration_point][coordinate_point] = field_bulk_data.localCoordSystemDouble[current_pointer++];
                        }
                    }
                    int total_points = current_position*field_bulk_data.width;
                    for (int component=0; component<field_bulk_data.width; ++component) {
                        ostringstream double_string;
                        double_string << std::scientific << field_bulk_data.dataDouble[total_points+1];
                        log_file.logDebug("element: " + to_string(element + 1) + " integration point: " + to_string(integration_point + 1) + " component: " + to_string(component + 1) + " double data: " + double_string.str());
                        data_double[element][integration_point][component] = field_bulk_data.dataDouble[total_points++];
                    }
                    if (complex_data) {
                        total_points = current_position*field_bulk_data.width;
                        for (int component=0; component<field_bulk_data.width; ++component) {
                            conjugate_data_double[element][integration_point][component] = field_bulk_data.conjugateDataDouble[total_points++];
                        }
                    }
                    if (!field_bulk_data.emptyMises) {
                        mises[element][integration_point] = field_bulk_data.mises[current_position];
                    }
                }
            }
            
//            write_double_3D_data(bulk_group, "data", field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements, *data_double);
//            write_double_3D_data(bulk_group, "conjugateData", field_bulk_data.width, number_of_integration_points, field_bulk_data.numberOfElements, *conjugate_data_double);
//            write_double_3D_data(bulk_group, "localCoordSystem", field_bulk_data.orientationWidth, number_of_integration_points, field_bulk_data.numberOfElements, *local_coordinate_system_double);
        }
           */
        if (!empty_faces) {
            write_string_2D_array(bulk_group, "faces", field_bulk_data.width, field_bulk_data.numberOfElements, *faces);
        }
        if (!field_bulk_data.mises.empty()) {
            write_float_2D_array(bulk_group, "mises", field_bulk_data.width, field_bulk_data.numberOfElements, *mises);
        }
        write_integer_2D_array(bulk_group, "elementLabels", field_bulk_data.width, field_bulk_data.numberOfElements, *element_labels);
        write_integer_2D_array(bulk_group, "integrationPoints", field_bulk_data.width, field_bulk_data.numberOfElements, *integration_points);
    } else {  // Nodes
    /*
        int node_labels[field_bulk_data.length];
        if(field_bulk_data.precision == "Single Precision") {
            float data[field_bulk_data.length][field_bulk_data.width];
            float conjugate_data[field_bulk_data.length][field_bulk_data.width];
            for (int node_count=0; node_count<field_bulk_data.length; ++node_count) {
                node_labels[node_count] = field_bulk_data.nodeLabels[node_count];
                int total_points = node_count*field_bulk_data.width;
                for (int component=0; component<field_bulk_data.width; ++component) {
                    data[node_count][component] = field_bulk_data.data[total_points++];
                }
                if (complex_data) {
                    total_points = node_count*field_bulk_data.width;
                    for (int component=0; component<field_bulk_data.width; ++component) {
                        conjugate_data[node_count][component] = field_bulk_data.conjugateData[total_points++];
                    }
                }
            }
            write_float_2D_array(bulk_group, "data", field_bulk_data.width, field_bulk_data.length, *data);
            write_float_2D_array(bulk_group, "conjugateData", field_bulk_data.width, field_bulk_data.length, *conjugate_data);
        } else {  // Double precision
            double data_double[field_bulk_data.length][field_bulk_data.width];
            double conjugate_data_double[field_bulk_data.length][field_bulk_data.width];
            for (int node_count=0; node_count<field_bulk_data.length; ++node_count) {
                node_labels[node_count] = field_bulk_data.nodeLabels[node_count];
                int total_points = node_count*field_bulk_data.width;
                for (int component=0; component<field_bulk_data.width; ++component) {
                    data_double[node_count][component] = field_bulk_data.data[total_points++];
                }
                if (complex_data) {
                    total_points = node_count*field_bulk_data.width;
                    for (int component=0; component<field_bulk_data.width; ++component) {
                        conjugate_data_double[node_count][component] = field_bulk_data.conjugateDataDouble[total_points++];
                    }
                }
            }
    */
            write_double_2D_array(bulk_group, "data", field_bulk_data.width, field_bulk_data.length, *data_double);
            write_double_2D_array(bulk_group, "conjugateData", field_bulk_data.width, field_bulk_data.length, *conjugate_data_double);
        }
//        write_integer_array_dataset(bulk_group, "nodeLabels", field_bulk_data.length, node_labels);
        write_integer_array_dataset(bulk_group, "nodeLabels", field_bulk_data.length, field_bulk_data.nodeLabels);
    }
}

void SpadeObject::write_field_output(H5::H5File &h5_file, Logging &log_file, const string &group_name, field_output_type &field_output) {
    H5::Group field_output_group = h5_file.createGroup(group_name.c_str());
    write_string_dataset(field_output_group, "name", field_output.name);
    write_string_dataset(field_output_group, "description", field_output.description);
    write_string_dataset(field_output_group, "type", field_output.type);
    write_integer_dataset(field_output_group, "dim", field_output.dim);
    write_integer_dataset(field_output_group, "dim2", field_output.dim2);
//    write_string_dataset(field_output_group, "isEngineeringTensor", field_output.isEngineeringTensor);
    write_string_vector_dataset(field_output_group, "componentLabels", field_output.componentLabels);
    write_string_vector_dataset(field_output_group, "validInvariants", field_output.validInvariants);

    H5::Group locations_group = h5_file.createGroup((group_name + "/locations").c_str());
    for (int i=0; i<field_output.locations.size(); i++) {
        string location_group_name = group_name + "/locations/" + to_string(i);
        H5::Group location_group = h5_file.createGroup(location_group_name.c_str());
        write_string_dataset(location_group, "position", field_output.locations[i].position);
        H5::Group section_points_group = h5_file.createGroup((location_group_name + "/sectionPoint").c_str());
        for (int j=0; j<field_output.locations[i].sectionPoint.size(); j++) {
            H5::Group section_point_group = h5_file.createGroup((location_group_name + "/sectionPoint/" + field_output.locations[i].sectionPoint[j].number).c_str());
            write_string_dataset(section_point_group, "description", field_output.locations[i].sectionPoint[j].description);
        }
    }
    // TODO: find out a way to speed up this process
    log_file.logDebug("Writing field output values for " + group_name + ".");
    H5::Group values_group = h5_file.createGroup((group_name + "/values").c_str());
    if (!field_output.node_values_empty) {
        H5::Group node_values_group = h5_file.createGroup((group_name + "/values/nodes").c_str());
        for (auto [node_number, field_output_value] : field_output.nodeValues) {
//        for (int i=0; i<field_output.nodeValues.size(); i++) {
            string value_group_name = group_name + "/values/nodes/" + to_string(node_number);
            write_field_value(h5_file, value_group_name, field_output_value);
        }
    }
    if (!field_output.element_values_empty) {
        H5::Group element_values_group = h5_file.createGroup((group_name + "/values/elements").c_str());
        for (auto [element_number, field_output_value] : field_output.elementValues) {
            string value_group_name = group_name + "/values/elements/" + to_string(element_number);
            write_field_value(h5_file, value_group_name, field_output_value);
        }
    }
    for (int i=0; i<field_output.dataValues.size(); i++) {
        string value_group_name = group_name + "/values/" + to_string(i);
        write_field_bulk_data(h5_file, log_file, value_group_name, field_output.dataValues[i], field_output.isComplex);
    }
    write_attribute(field_output_group, "max_width", to_string(field_output.max_width));
    write_attribute(field_output_group, "max_length", to_string(field_output.max_length));
}

void SpadeObject::write_frames(H5::H5File &h5_file, Logging &log_file, const string &group_name, vector<frame_type> &frames) {
    string frames_group_name = group_name + "/frames";
    H5::Group frames_group = h5_file.createGroup(frames_group_name.c_str());
    for (auto frame : frames) {
        string frame_group_name = frames_group_name + "/" + to_string(frame.incrementNumber);
        H5::Group frame_group = h5_file.createGroup(frame_group_name.c_str());
        write_integer_dataset(frame_group, "cyclicModeNumber", frame.cyclicModeNumber);
        write_integer_dataset(frame_group, "mode", frame.mode);
        write_string_dataset(frame_group, "description", frame.description);
        write_string_dataset(frame_group, "domain", frame.domain);
        write_string_dataset(frame_group, "loadCase", frame.loadCase);
        write_float_dataset(frame_group, "frameValue", frame.frameValue);
        write_float_dataset(frame_group, "frequency", frame.frequency);
        H5::Group field_outputs_group = h5_file.createGroup((frame_group_name + "/fieldOutputs").c_str());
        log_file.logVerbose("Writing field output.");
        for (int i=0; i<frame.fieldOutputs.size(); i++) {
            string field_output_group_name = frame_group_name + "/fieldOutputs/" + to_string(i);
            write_field_output(h5_file, log_file, field_output_group_name, frame.fieldOutputs[i]);
        }
        write_attribute(frame_group, "max_width", to_string(frame.max_width));
        write_attribute(frame_group, "max_length", to_string(frame.max_length));
    }
}
void SpadeObject::write_history_point(H5::H5File &h5_file, const string &group_name, history_point_type &history_point) {
    string history_point_group_name = group_name + "/point";
    H5::Group history_point_group = h5_file.createGroup(history_point_group_name.c_str());
    write_string_dataset(history_point_group, "face", history_point.face);
    write_string_dataset(history_point_group, "position", history_point.position);
    if (history_point.hasElement) {
        string element_group_name = history_point_group_name + "/element";
        H5::Group element_group = h5_file.createGroup(element_group_name.c_str());
        write_element(h5_file, element_group_name, *history_point.element);
    }
    if (history_point.hasNode) {
        string node_group_name = history_point_group_name + "/node";
        H5::Group node_group = h5_file.createGroup(node_group_name.c_str());
        write_node(h5_file, node_group, node_group_name, *history_point.node);
    }
    write_set(h5_file, history_point_group_name, history_point.region);
    write_string_dataset(history_point_group, "assembly", history_point.assemblyName);
    write_string_dataset(history_point_group, "instance", history_point.instanceName);
    write_integer_dataset(history_point_group, "ipNumber", history_point.ipNumber);
    H5::Group section_point_group = h5_file.createGroup((history_point_group_name + "/sectionPoint").c_str());
    write_string_dataset(section_point_group, "number", history_point.sectionPoint.number);
    write_string_dataset(section_point_group, "description", history_point.sectionPoint.description);

}

void SpadeObject::write_history_output(H5::H5File &h5_file, const string &group_name, history_output_type &history_output) {
    H5::Group history_output_group = h5_file.createGroup(group_name.c_str());
    write_string_dataset(history_output_group, "name", history_output.name);
    write_string_dataset(history_output_group, "description", history_output.description);
    write_string_dataset(history_output_group, "type", history_output.type);
    write_float_2D_vector(history_output_group, "data", history_output.max_column_size, history_output.data);
    write_float_2D_vector(history_output_group, "conjugateData", history_output.max_column_size_conjugate, history_output.conjugateData);
}

void SpadeObject::write_history_regions(H5::H5File &h5_file, const string &group_name, vector<history_region_type> &history_regions) {
    string history_regions_group_name = group_name + "/historyRegions";
    H5::Group history_regions_group = h5_file.createGroup(history_regions_group_name.c_str());
    for (auto history_region : history_regions) {
        string history_region_group_name = history_regions_group_name + "/" + history_region.name;
        H5::Group history_region_group = h5_file.createGroup(history_region_group_name.c_str());
        write_string_dataset(history_region_group, "description", history_region.description);
        write_string_dataset(history_region_group, "position", history_region.position);
        write_string_dataset(history_region_group, "loadCase", history_region.loadCase);
        write_history_point(h5_file, history_region_group_name, history_region.point);
        H5::Group history_outputs_group = h5_file.createGroup((history_region_group_name + "/historyOutputs").c_str());
        for (int i=0; i<history_region.historyOutputs.size(); i++) {
            string history_output_group_name = history_region_group_name + "/historyOutputs/" + to_string(i);
            write_history_output(h5_file, history_output_group_name, history_region.historyOutputs[i]);
        }
        history_region.historyOutputs = vector<history_output_type>();  // set the vector to an empty vector - i.e. clear the memory
    }
}

void SpadeObject::write_steps(H5::H5File &h5_file, Logging &log_file, const string &group_name) {
    string steps_group_name = group_name + "/steps";
    H5::Group steps_group = h5_file.createGroup(steps_group_name.c_str());
    for (auto step : this->steps) {
        string step_group_name = steps_group_name + "/" + step.name;
        H5::Group step_group = h5_file.createGroup(step_group_name.c_str());
        write_string_dataset(step_group, "description", step.description);
        write_string_dataset(step_group, "domain", step.domain);
        write_string_dataset(step_group, "previousStepName", step.previousStepName);
        write_string_dataset(step_group, "procedure", step.procedure);
        write_string_dataset(step_group, "nlgeom", step.nlgeom);
        write_integer_dataset(step_group, "number", step.number);
        write_double_dataset(step_group, "timePeriod", step.timePeriod);
        write_double_dataset(step_group, "totalTime", step.totalTime);
        write_double_dataset(step_group, "mass", step.mass);
        write_double_dataset(step_group, "acousticMass", step.acousticMass);
        write_string_vector_dataset(step_group, "loadCases", step.loadCases);
        write_double_vector_dataset(step_group, "massCenter", step.massCenter);
        write_double_vector_dataset(step_group, "acousticMassCenter", step.acousticMassCenter);
        write_double_array_dataset(step_group, "inertiaAboutCenter", 6, step.inertiaAboutCenter);
        write_double_array_dataset(step_group, "inertiaAboutOrigin", 6, step.inertiaAboutOrigin);
        log_file.logVerbose("Writing frames data.");
        write_frames(h5_file, log_file, step_group_name, step.frames);
        log_file.logVerbose("Writing history data.");
        write_history_regions(h5_file, step_group_name, step.historyRegions);
    }
}

void SpadeObject::write_instances(H5::H5File &h5_file, const string &group_name) {
    string instances_group_name = group_name + "/instances";
    H5::Group instances_group = h5_file.createGroup(instances_group_name.c_str());
    for (auto instance : this->root_assembly.instances) {
        string instance_group_name = instances_group_name + "/" + instance.name;
        H5::Group instance_group = h5_file.createGroup(instance_group_name.c_str());
        write_string_dataset(instance_group, "embeddedSpace", instance.embeddedSpace);
        write_nodes(h5_file, instance_group_name, instance.nodes);
        write_elements(h5_file, instance_group_name, instance.elements);
        write_sets(h5_file, instance_group_name + "/nodeSets", instance.nodeSets);
        write_sets(h5_file, instance_group_name + "/elementSets", instance.elementSets);
        write_sets(h5_file, instance_group_name + "/surfaces", instance.surfaces);
        this->instance_links[instance.name] = instance_group_name;
        H5::Group section_assignments_group = h5_file.createGroup((instance_group_name + "/sectionAssignments").c_str());
        for (int i=0; i<instance.sectionAssignments.size(); i++) {
            string section_assignment_group_name = instance_group_name + "/sectionAssignments/" + to_string(i);
            H5::Group section_assignment_group = h5_file.createGroup(section_assignment_group_name.c_str());
            write_set(h5_file, section_assignment_group_name, instance.sectionAssignments[i].region);
            write_string_dataset(section_assignment_group, "sectionName", instance.sectionAssignments[i].sectionName);
        }
        H5::Group rigid_bodies_group = h5_file.createGroup((instance_group_name + "/rigidBodies").c_str());
        for (int i=0; i<instance.rigidBodies.size(); i++) {
            string rigid_body_group_name = instance_group_name + "/rigidBodies/" + to_string(i);
            H5::Group rigid_body_group = h5_file.createGroup(rigid_body_group_name.c_str());
            write_string_dataset(rigid_body_group, "position", instance.rigidBodies[i].position);
            write_string_dataset(rigid_body_group, "isothermal", instance.rigidBodies[i].isothermal);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].referenceNode);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].elements);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].tieNodes);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].pinNodes);
            write_analytic_surface(h5_file, rigid_body_group_name, instance.rigidBodies[i].analyticSurface);
        }
        H5::Group beam_orientations_group = h5_file.createGroup((instance_group_name + "/beamOrientations").c_str());
        for (int i=0; i<instance.beamOrientations.size(); i++) {
            string beam_orientation_group_name = instance_group_name + "/beamOrientations/" + to_string(i);
            H5::Group beam_orientation_group = h5_file.createGroup(beam_orientation_group_name.c_str());
            write_set(h5_file, beam_orientation_group_name, instance.beamOrientations[i].region);
            write_string_dataset(beam_orientation_group, "method", instance.beamOrientations[i].method);
            write_float_vector_dataset(beam_orientation_group, "vector", instance.beamOrientations[i].beam_vector);
        }
        H5::Group rebar_orientations_group = h5_file.createGroup((instance_group_name + "/rebarOrientations").c_str());
        for (int i=0; i<instance.rebarOrientations.size(); i++) {
            string rebar_orientation_group_name = instance_group_name + "/rebarOrientations/" + to_string(i);
            H5::Group rebar_orientation_group = h5_file.createGroup(rebar_orientation_group_name.c_str());
            write_string_dataset(rebar_orientation_group, "axis", instance.rebarOrientations[i].axis);
            write_float_dataset(rebar_orientation_group, "angle", instance.rebarOrientations[i].angle);
            write_set(h5_file, rebar_orientation_group_name, instance.rebarOrientations[i].region);
            write_datum_csys(h5_file, rebar_orientation_group_name, instance.rebarOrientations[i].csys);
        }
        write_analytic_surface(h5_file, instance_group_name, instance.analyticSurface);
    }
}

void SpadeObject::write_analytic_surface(H5::H5File &h5_file, const string &group_name, const analytic_surface_type &analytic_surface) {
    string analytic_surface_group_name = group_name + "/analyticSurface";
    H5::Group surface_group = h5_file.createGroup(analytic_surface_group_name.c_str());
    write_string_dataset(surface_group, "name", analytic_surface.name);
    write_string_dataset(surface_group, "type", analytic_surface.type);
    write_double_dataset(surface_group, "filletRadius", analytic_surface.filletRadius);
    H5::Group segments_group = h5_file.createGroup((analytic_surface_group_name + "/segments").c_str());
    for (int i=0; i<analytic_surface.segments.size(); i++) {
        string segment_group_name = analytic_surface_group_name + "/segments/" + to_string(i);
        H5::Group segment_group = h5_file.createGroup(segment_group_name.c_str());
        write_string_dataset(segment_group, "type", analytic_surface.segments[i].type);
        write_float_2D_vector(segment_group, "data", analytic_surface.segments[i].max_column_size, analytic_surface.segments[i].data);
    }
    write_float_2D_vector(surface_group, "localCoordData", analytic_surface.max_column_size, analytic_surface.localCoordData);
}

void SpadeObject::write_datum_csys(H5::H5File &h5_file, const string &group_name, const datum_csys_type &datum_csys) {
    string datum_group_name = group_name + "/csys";
    H5::Group datum_group = h5_file.createGroup(datum_group_name.c_str());
    write_string_dataset(datum_group, "name", datum_csys.name);
    write_string_dataset(datum_group, "type", datum_csys.type);
    write_float_array_dataset(datum_group, "xAxis", 3, datum_csys.x_axis);
    write_float_array_dataset(datum_group, "yAxis", 3, datum_csys.y_axis);
    write_float_array_dataset(datum_group, "zAxis", 3, datum_csys.z_axis);
    write_float_array_dataset(datum_group, "origin", 3, datum_csys.origin);
}

void SpadeObject::write_constraints(H5::H5File &h5_file, const string &group_name) {
    if (!this->constraints.ties.empty()) {
        H5::Group ties_group = h5_file.createGroup((group_name + "/tie").c_str());
        for (int i=0; i<this->constraints.ties.size(); i++) {
            string tie_group_name = group_name + "/tie/" + to_string(i);
            H5::Group tie_group = h5_file.createGroup(tie_group_name.c_str());
            write_set(h5_file, tie_group_name, this->constraints.ties[i].main);
            write_set(h5_file, tie_group_name, this->constraints.ties[i].secondary);
            write_string_dataset(tie_group, "adjust", this->constraints.ties[i].adjust);
            write_string_dataset(tie_group, "tieRotations", this->constraints.ties[i].tieRotations);
            write_string_dataset(tie_group, "positionToleranceMethod", this->constraints.ties[i].positionToleranceMethod);
            write_string_dataset(tie_group, "positionTolerance", this->constraints.ties[i].positionTolerance);
            write_string_dataset(tie_group, "constraintRatioMethod", this->constraints.ties[i].constraintRatioMethod);
            write_string_dataset(tie_group, "constraintRatio", this->constraints.ties[i].constraintRatio);
            write_string_dataset(tie_group, "constraintEnforcement", this->constraints.ties[i].constraintEnforcement);
            write_string_dataset(tie_group, "thickness", this->constraints.ties[i].thickness);
        }
    }
    if (!this->constraints.display_bodies.empty()) {
        H5::Group display_bodies_group = h5_file.createGroup((group_name + "/displayBody").c_str());
        for (int i=0; i<this->constraints.display_bodies.size(); i++) {
            string display_body_group_name = group_name + "/displayBody/" + to_string(i);
            H5::Group display_body_group = h5_file.createGroup(display_body_group_name.c_str());
            write_string_dataset(display_body_group, "instanceName", this->constraints.display_bodies[i].instanceName);
            write_string_dataset(display_body_group, "referenceNode1InstanceName", this->constraints.display_bodies[i].referenceNode1InstanceName);
            write_string_dataset(display_body_group, "referenceNode1Label", this->constraints.display_bodies[i].referenceNode1Label);
            write_string_dataset(display_body_group, "referenceNode2InstanceName", this->constraints.display_bodies[i].referenceNode2InstanceName);
            write_string_dataset(display_body_group, "referenceNode2Label", this->constraints.display_bodies[i].referenceNode2Label);
            write_string_dataset(display_body_group, "referenceNode3InstanceName", this->constraints.display_bodies[i].referenceNode3InstanceName);
            write_string_dataset(display_body_group, "referenceNode3Label", this->constraints.display_bodies[i].referenceNode3Label);
        }
    }
    if (!this->constraints.couplings.empty()) {
        H5::Group couplings_group = h5_file.createGroup((group_name + "/coupling").c_str());
        for (int i=0; i<this->constraints.couplings.size(); i++) {
            string coupling_group_name = group_name + "/coupling/" + to_string(i);
            H5::Group coupling_group = h5_file.createGroup(coupling_group_name.c_str());
            write_set(h5_file, coupling_group_name, this->constraints.couplings[i].surface);
            write_set(h5_file, coupling_group_name, this->constraints.couplings[i].refPoint);
            write_set(h5_file, coupling_group_name, this->constraints.couplings[i].nodes);
            write_string_dataset(coupling_group, "couplingType", this->constraints.couplings[i].couplingType);
            write_string_dataset(coupling_group, "weightingMethod", this->constraints.couplings[i].weightingMethod);
            write_string_dataset(coupling_group, "influenceRadius", this->constraints.couplings[i].influenceRadius);
            write_string_dataset(coupling_group, "u1", this->constraints.couplings[i].u1);
            write_string_dataset(coupling_group, "u2", this->constraints.couplings[i].u2);
            write_string_dataset(coupling_group, "u3", this->constraints.couplings[i].u3);
            write_string_dataset(coupling_group, "ur1", this->constraints.couplings[i].ur1);
            write_string_dataset(coupling_group, "ur2", this->constraints.couplings[i].ur2);
            write_string_dataset(coupling_group, "ur3", this->constraints.couplings[i].ur3);
        }
    }
    if (!this->constraints.mpc.empty()) {
        H5::Group mpcs_group = h5_file.createGroup((group_name + "/mpc").c_str());
        for (int i=0; i<this->constraints.mpc.size(); i++) {
            string mpc_group_name = group_name + "/mpc/" + to_string(i);
            H5::Group mpc_group = h5_file.createGroup(mpc_group_name.c_str());
            write_set(h5_file, mpc_group_name, this->constraints.mpc[i].surface);
            write_set(h5_file, mpc_group_name, this->constraints.mpc[i].refPoint);
            write_string_dataset(mpc_group, "mpcType", this->constraints.mpc[i].mpcType);
            write_string_dataset(mpc_group, "userMode", this->constraints.mpc[i].userMode);
            write_string_dataset(mpc_group, "userType", this->constraints.mpc[i].userType);
        }
    }
    if (!this->constraints.shell_solid_couplings.empty()) {
        H5::Group shell_solid_couplings_group = h5_file.createGroup((group_name + "/shellSolidCoupling").c_str());
        for (int i=0; i<this->constraints.shell_solid_couplings.size(); i++) {
            string shell_solid_coupling_group_name = group_name + "/shellSolidCoupling/" + to_string(i);
            H5::Group shell_solid_coupling_group = h5_file.createGroup(shell_solid_coupling_group_name.c_str());
            write_set(h5_file, shell_solid_coupling_group_name, this->constraints.shell_solid_couplings[i].shellEdge);
            write_set(h5_file, shell_solid_coupling_group_name, this->constraints.shell_solid_couplings[i].solidFace);
            write_string_dataset(shell_solid_coupling_group, "positionToleranceMethod", this->constraints.shell_solid_couplings[i].positionToleranceMethod);
            write_string_dataset(shell_solid_coupling_group, "positionTolerance", this->constraints.shell_solid_couplings[i].positionTolerance);
            write_string_dataset(shell_solid_coupling_group, "influenceDistanceMethod", this->constraints.shell_solid_couplings[i].influenceDistanceMethod);
            write_string_dataset(shell_solid_coupling_group, "influenceDistance", this->constraints.shell_solid_couplings[i].influenceDistance);
        }
    }
}

void SpadeObject::write_tangential_behavior(H5::H5File &h5_file, const string &group_name, const tangential_behavior_type& tangential_behavior) {
    H5::Group tangential_behavior_group = h5_file.createGroup((group_name + "/tangentialBehavior").c_str());
    write_string_dataset(tangential_behavior_group, "formulation", tangential_behavior.formulation);
    write_string_dataset(tangential_behavior_group, "directionality", tangential_behavior.directionality);
    write_string_dataset(tangential_behavior_group, "slipRateDependency", tangential_behavior.slipRateDependency);
    write_string_dataset(tangential_behavior_group, "pressureDependency", tangential_behavior.pressureDependency);
    write_string_dataset(tangential_behavior_group, "temperatureDependency", tangential_behavior.temperatureDependency);
    write_string_dataset(tangential_behavior_group, "exponentialDecayDefinition", tangential_behavior.exponentialDecayDefinition);
    write_string_dataset(tangential_behavior_group, "maximumElasticSlip", tangential_behavior.maximumElasticSlip);
    write_string_dataset(tangential_behavior_group, "useProperties", tangential_behavior.useProperties);
    write_integer_dataset(tangential_behavior_group, "dependencies", tangential_behavior.dependencies);
    write_integer_dataset(tangential_behavior_group, "nStateDependentVars", tangential_behavior.nStateDependentVars);
    write_double_dataset(tangential_behavior_group, "fraction", tangential_behavior.fraction);
    write_double_dataset(tangential_behavior_group, "shearStressLimit", tangential_behavior.shearStressLimit);
    write_double_dataset(tangential_behavior_group, "absoluteDistance", tangential_behavior.absoluteDistance);
    write_double_dataset(tangential_behavior_group, "elasticSlipStiffness", tangential_behavior.elasticSlipStiffness);
    write_double_2D_vector(tangential_behavior_group, "table", tangential_behavior.max_column_size, tangential_behavior.table);
}

void SpadeObject::write_interactions(H5::H5File &h5_file, const string &group_name) {
    H5::Group interactions_group = h5_file.createGroup((group_name + "/interactions").c_str());
    if (!this->standard_interactions.empty()) {
        H5::Group interactions_group = h5_file.createGroup((group_name + "/interactions/standard").c_str());
        for (int i=0; i<this->standard_interactions.size(); i++) {
            string standard_group_name = group_name + "/interactions/standard/" + to_string(i);
            H5::Group standards_group = h5_file.createGroup(standard_group_name.c_str());
            write_string_dataset(standards_group, "sliding", this->standard_interactions[i].sliding);
            write_string_dataset(standards_group, "limitSlideDistance", this->standard_interactions[i].limitSlideDistance);
            write_string_dataset(standards_group, "adjustMethod", this->standard_interactions[i].adjustMethod);
            write_string_dataset(standards_group, "enforcement", this->standard_interactions[i].enforcement);
            write_string_dataset(standards_group, "thickness", this->standard_interactions[i].thickness);
            write_string_dataset(standards_group, "tied", this->standard_interactions[i].tied);
            write_string_dataset(standards_group, "contactTracking", this->standard_interactions[i].contactTracking);
            write_string_dataset(standards_group, "createStepName", this->standard_interactions[i].createStepName);
            write_double_dataset(standards_group, "smooth", this->standard_interactions[i].smooth);
            write_double_dataset(standards_group, "hcrit", this->standard_interactions[i].hcrit);
            write_double_dataset(standards_group, "slideDistance", this->standard_interactions[i].slideDistance);
            write_double_dataset(standards_group, "extensionZone", this->standard_interactions[i].extensionZone);
            write_double_dataset(standards_group, "adjustTolerance", this->standard_interactions[i].adjustTolerance);
            write_tangential_behavior(h5_file, standard_group_name, this->standard_interactions[i].interactionProperty);
        }
    }
    if (!this->explicit_interactions.empty()) {
        H5::Group interactions_group = h5_file.createGroup((group_name + "/interactions/explicit").c_str());
        for (int i=0; i<this->explicit_interactions.size(); i++) {
            string explicit_group_name = group_name + "/interactions/explicit/" + to_string(i);
            H5::Group explicit_group = h5_file.createGroup(explicit_group_name.c_str());
            write_string_dataset(explicit_group, "sliding", this->explicit_interactions[i].sliding);
            write_string_dataset(explicit_group, "mainNoThick", this->explicit_interactions[i].mainNoThick);
            write_string_dataset(explicit_group, "secondaryNoThick", this->explicit_interactions[i].secondaryNoThick);
            write_string_dataset(explicit_group, "mechanicalConstraint", this->explicit_interactions[i].mechanicalConstraint);
            write_string_dataset(explicit_group, "weightingFactorType", this->explicit_interactions[i].weightingFactorType);
            write_string_dataset(explicit_group, "createStepName", this->explicit_interactions[i].createStepName);
            write_string_dataset(explicit_group, "useReverseDatumAxis", this->explicit_interactions[i].useReverseDatumAxis);
            write_string_dataset(explicit_group, "contactControls", this->explicit_interactions[i].contactControls);
            write_double_dataset(explicit_group, "weightingFactor", this->explicit_interactions[i].weightingFactor);
            write_tangential_behavior(h5_file, explicit_group_name, this->explicit_interactions[i].interactionProperty);
            write_set(h5_file, explicit_group_name, this->explicit_interactions[i].main);
            write_set(h5_file, explicit_group_name, this->explicit_interactions[i].secondary);
        }
    }
}

void SpadeObject::write_element(H5::H5File &h5_file, const string &group_name, const element_type &element) {
    string element_link;
    string newGroupName = group_name + "/" + to_string(element.label);
    string element_key;
    for (int i=0; i < element.instanceNames.size(); i++) {
        element_key += element.instanceNames[i];
    }
    element_key = to_string(element.label) + element_key;
    try {
        element_link = this->element_links.at(element_key);
        h5_file.link(H5L_TYPE_SOFT, element_link, newGroupName);
    } catch (const std::out_of_range& oor) {
        H5::Group element_group = h5_file.createGroup((group_name + "/" + to_string(element.label)).c_str());
        write_string_dataset(element_group, "type", element.type);
        write_integer_vector_dataset(element_group, "connectivity", element.connectivity);
        H5::Group section_category_group = h5_file.createGroup((group_name + "/" + to_string(element.label) + "/sectionCategory").c_str());
        write_section_category(h5_file, section_category_group, group_name + "/" + to_string(element.label) + "/sectionCategory", element.sectionCategory);
        write_string_vector_dataset(element_group, "instanceNames", element.instanceNames);

        this->element_links[element_key] = newGroupName;  // Store link for later
    }

}

void SpadeObject::write_elements(H5::H5File &h5_file, const string &group_name, const vector<element_type*> &elements) {
    if (!elements.empty()) {
        H5::Group elements_group = h5_file.createGroup((group_name + "/elements").c_str());
        for (auto element : elements) { write_element(h5_file, group_name + "/elements", *element); }
    }
}

void SpadeObject::write_node(H5::H5File &h5_file, H5::Group &group, const string &group_name, const node_type &node) {
    string node_link;
    string newGroupName = group_name + "/" + to_string(node.label);
    std::stringstream floatS;
    floatS << std::noshowpos << std::setprecision(7) << node.label << "_" << node.coordinates[0] << "_" << node.coordinates[0] << "_" << node.coordinates[0];
    string node_key(floatS.str());  // Needed to set the maximum precision of a float (7) to make sure the string is unique

    try {
        node_link = this->node_links.at(node_key);
        // If link is found, then write a link rather than all the data again
        hsize_t dimensions[] = {1};
        H5::DataSpace dataspace(1, dimensions);
        try {
            Exception::dontPrint();
            h5_file.link(H5L_TYPE_SOFT, node_link, newGroupName);
        } catch (FileIException error) {
//            cout << error.getDetailMsg() << endl; // "creating link failed"
//            log_file.warning(error.getDetailMsg());
        }
    } catch (const std::out_of_range& oor) {  // If node.label is not found in the node_links map
        hsize_t dimensions[] = {3};
        H5::DataSpace dataspace(1, dimensions);
        H5::DataSet dataset = group.createDataSet(newGroupName, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(node.coordinates, H5::PredType::NATIVE_FLOAT);
        this->node_links[node_key] = newGroupName;  // Store link for later
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_nodes(H5::H5File &h5_file, const string &group_name, const vector<node_type*> &nodes) {
    if (!nodes.empty()) {
        H5::Group nodes_group = h5_file.createGroup((group_name + "/nodes").c_str());
        for (auto node : nodes) { write_node(h5_file, nodes_group, group_name + "/nodes", *node); }
    }
}

void SpadeObject::write_sets(H5::H5File &h5_file, const string &group_name, const vector<set_type> &sets) {
    if (!sets.empty()) {
        H5::Group sets_group = h5_file.createGroup(group_name.c_str());
        for (auto set : sets) { write_set(h5_file, group_name, set); }
    }
}

void SpadeObject::write_set(H5::H5File &h5_file, const string &group_name, const set_type &set) {
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

void SpadeObject::write_section_category(H5::H5File &h5_file, const H5::Group &group, const string &group_name, const section_category_type &section_category) {
    write_string_dataset(group, "description", section_category.description);
    for (int j=0; j<section_category.sectionPoints.size(); j++) {
        string point_group_name = group_name + "/" + section_category.sectionPoints[j].number;
        H5::Group section_point_group = h5_file.createGroup(point_group_name.c_str());
        write_string_dataset(section_point_group, "description", section_category.sectionPoints[j].description);
    }
}

void SpadeObject::write_attribute(const H5::Group& group, const string & attribute_name, const string & string_value) {
    H5::DataSpace attribute_space(H5S_SCALAR);
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);  // Use the length of the string or 1 if string is blank
    H5::Attribute attribute = group.createAttribute(attribute_name, string_type, attribute_space);
    attribute.write(string_type, string_value);
    attribute_space.close();
}

void SpadeObject::write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value) {
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

void SpadeObject::write_string_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<string> & string_values) {
    if (!string_values.empty()) {
        std::vector<const char*> c_string_array;
        for (int i = 0; i < string_values.size(); ++i) {
            c_string_array.push_back(string_values[i].c_str());
        }
        hsize_t dimensions[1] {c_string_array.size()};
        H5::DataSpace  dataspace(1, dimensions);
        H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE); // Variable length string
        H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
        dataset.write(c_string_array.data(), string_type);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_string_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, string *string_array) {
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::C_S1, dataspace);
    dataset.write(string_array, H5::PredType::C_S1);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_string_2D_vector(H5::H5File &h5_file, const H5::Group& group, const string & dataset_name, const int & max_column_size, const vector<vector<string>> & string_data) {
    if (!string_data.empty()) {
        // TODO: This is not an ideal solution, but I tried so many ways to write a 2 dimensional vector of strings to hdf5 file and I couldn't figure it out
        // This solution writes a separate datset for each row of the data
        H5::Group dataset_group = h5_file.createGroup(dataset_name.c_str());
        for( int i = 0; i<string_data.size(); ++i) {
            write_string_vector_dataset(dataset_group, to_string(i), string_data[i]);
        }
    }
}

void SpadeObject::write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::STD_I32BE, dataspace);
    dataset.write(&int_value, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_integer_array_dataset(const H5::Group& group, const string & dataset_name, const int array_size, const int* int_array) {
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
    dataset.write(int_array, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_integer_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<int> & int_data) {
    if (!int_data.empty()) {
        hsize_t dimensions[] = {int_data.size()};
        H5::DataSpace dataspace(1, dimensions);
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        dataset.write(int_data.data(), H5::PredType::NATIVE_INT);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_integer_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, int *integer_array) {
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
    dataset.write(integer_array, H5::PredType::NATIVE_INT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_integer_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, const vector<vector<int>> & integer_data) {
    if (!integer_data.empty()) {
        hsize_t dimensions[] = {integer_data.size(), max_column_size};
        H5::DataSpace dataspace(2, dimensions);  // two dimensional data
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        /*
        for (auto & row : integer_data) {
            dataset.write(row.data(), H5::PredType::NATIVE_INT);
        }
        */
        dataset.write(integer_data.data(), H5::PredType::NATIVE_INT);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_float_dataset(const H5::Group &group, const string &dataset_name, const float &float_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(&float_value, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_float_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const float* float_array) {
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_float_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<float> &float_data) {
    if (!float_data.empty()) {
        hsize_t dimensions[] = {float_data.size()};
        H5::DataSpace dataspace(1, dimensions);
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_data.data(), H5::PredType::NATIVE_FLOAT);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_float_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, float *float_array) {
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_float_3D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &aisle_size, const int &column_size, const vector<float *> &float_data) {
    hsize_t dimensions[] = {row_size, aisle_size, column_size};
    H5::DataSpace dataspace(3, dimensions);  // three dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
    dataset.write(float_data.data(), H5::PredType::NATIVE_FLOAT);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_float_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, const vector<vector<float>> & float_data) {
    if (!float_data.empty()) {
        hsize_t dimensions[] = {float_data.size(), max_column_size};
        H5::DataSpace dataspace(2, dimensions);  // two dimensional data
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_data.data(), H5::PredType::NATIVE_FLOAT);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_double_dataset(const H5::Group &group, const string &dataset_name, const double &double_value) {
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(&double_value, H5::PredType::NATIVE_DOUBLE);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_double_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const double* double_array) {
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(double_array, H5::PredType::NATIVE_DOUBLE);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_double_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<double> &double_data) {
    if (!double_data.empty()) {
        hsize_t dimensions[] = {double_data.size()};
        H5::DataSpace dataspace(1, dimensions);
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_data.data(), H5::PredType::NATIVE_DOUBLE);
        dataset.close();
        dataspace.close();
    }
}

void SpadeObject::write_double_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, double *double_array) {
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(double_array, H5::PredType::NATIVE_DOUBLE);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_double_3D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &aisle_size, const int &column_size, const vector<double *> &double_data) {
    hsize_t dimensions[] = {row_size, aisle_size, column_size};
    H5::DataSpace dataspace(3, dimensions);  // three dimensional data
    H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(double_data.data(), H5::PredType::NATIVE_DOUBLE);
    dataset.close();
    dataspace.close();
}

void SpadeObject::write_double_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, const vector<vector<double>> & double_data) {
    if (!double_data.empty()) {
        hsize_t dimensions[] = {double_data.size(), max_column_size};
        H5::DataSpace dataspace(2, dimensions);  // two dimensional data
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_data.data(), H5::PredType::NATIVE_DOUBLE);
        dataset.close();
        dataspace.close();
    }
}


void SpadeObject::write_yaml (CmdLineArguments &command_line_arguments, Logging &log_file) {
}

void SpadeObject::write_json (CmdLineArguments &command_line_arguments, Logging &log_file) {
}