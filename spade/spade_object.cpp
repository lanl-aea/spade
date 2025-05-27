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
#include <array>
#include <iterator>
#include <regex>
#include <stdlib.h>
#include <dirent.h>
#include <cmath>
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
    log_file.log("Reading file at time: " + command_line_arguments.getTimeStamp(false));
    log_file.log("Reading file: " + command_line_arguments["odb-file"]);
    odb_String file_name = command_line_arguments["odb-file"].c_str();

    if (isUpgradeRequiredForOdb(file_name)) {
        log_file.log("Upgrading file:" + command_line_arguments["odb-file"]);
        odb_String upgraded_file_name = std::filesystem::path(command_line_arguments["odb-file"]).replace_extension(".upgraded.odb").generic_string().c_str();
        upgradeOdb(file_name, upgraded_file_name);
        file_name = upgraded_file_name;
    }
    // Set up enum strings to be used with multiple functions later
    // Found at: /apps/SIMULIA/EstProducts/2023/SMAOdb/PublicInterfaces/odb_Enum.h
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
    this->default_instance_name = "ASSEMBLY";  // If the name of the instance is blank, it will be assigned this value

    this->command_line_arguments = &command_line_arguments;
    this->log_file = &log_file;
    try {  // Since the odb object isn't recognized outside the scope of the try/except, block the processing has to be done within the try block
        odb_Odb& odb = openOdb(file_name, true);  // Open as read only
        process_odb_without_steps(odb);
        log_file.log("Non step data from the odb processed and stored.");
        log_file.log("Writing extracted file at time: " + command_line_arguments.getTimeStamp(false));
        if (command_line_arguments["extracted-file-type"] == "h5") {
            // Open file for writing
            std::ifstream hdf5File (this->command_line_arguments->get("extracted-file").c_str());
            this->log_file->log("Creating hdf5 file: " + this->command_line_arguments->get("extracted-file"));
            const H5std_string FILE_NAME(this->command_line_arguments->get("extracted-file"));

            H5::Exception::dontPrint();
            H5::H5File* h5_file_pointer = 0;
            try {
                h5_file_pointer = new H5::H5File(FILE_NAME, H5F_ACC_TRUNC);
            } catch(const H5::FileIException&) {
                throw std::runtime_error("Issue opening file: " + this->command_line_arguments->get("extracted-file"));
            }
            H5::H5File h5_file = *h5_file_pointer;

            this->write_h5_without_steps(h5_file);
            if (command_line_arguments["format"] == "extract") {  //Write extract format
                write_mesh(h5_file);
                write_step_data_h5 (odb, h5_file);
            } else if (command_line_arguments["format"] == "vtk") {  //Write vtk format
                this->write_vtk_data(h5_file);
            }

            h5_file.close();  // Close the hdf5 file
            this->log_file->log("Closing hdf5 file.");
        } else if (command_line_arguments["extracted-file-type"] == "json") {
            this->write_json_without_steps();
        } else if (command_line_arguments["extracted-file-type"] == "yaml") {
            this->write_yaml_without_steps();
        }
        odb.close();
    }
    catch(odb_BaseException& exc) {
        string error_message = exc.UserReport().CStr();
        log_file.logErrorAndExit("odbBaseException caught. Abaqus error message: " + error_message);
    }
    catch (const std::exception& e) {
        // Catch any exception that inherits from std::exception
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
    /*
    catch(...) {
        log_file.logErrorAndExit("Unknown exception ");
    }
    */


}

void SpadeObject::process_odb_without_steps(odb_Odb &odb) {

    this->log_file->logVerbose("Reading top level attributes of odb.");
    this->name = odb.name().CStr();
    this->analysisTitle = odb.analysisTitle().CStr();
    this->description = odb.description().CStr();
    this->path = odb.path().CStr();
    this->isReadOnly = (odb.isReadOnly()) ? "true" : "false";

    // TODO: potentially figure out way to get amplitudes, filters, or materials

    this->log_file->logVerbose("Reading odb jobData.");
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
        this->log_file->logVerbose("Reading odb sector definition.");
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

    this->log_file->logVerbose("Reading odb section category data.");
    odb_SectionCategoryRepositoryIT section_category_iter(odb.sectionCategories());
    for (section_category_iter.first(); !section_category_iter.isDone(); section_category_iter.next()) {
        odb_SectionCategory section_category = section_category_iter.currentValue();
        this->section_categories.push_back(process_section_category(section_category));
    }

    this->log_file->logVerbose("Reading odb user data.");
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
        user_xy_data.row_size = user_xy_data_data.size();
        for (int i=0; i<user_xy_data_data.size(); i++) {
            odb_SequenceFloat user_xy_data_data_dimension1 = user_xy_data_data[i];
            for (int j=0; j<user_xy_data_data_dimension1.size(); j++) {
                user_xy_data.data.push_back(user_xy_data_data_dimension1[j]);
            }
        }
        this->user_xy_data.push_back(user_xy_data);
    }

    this->log_file->logVerbose("Reading odb interactions.");
    odb_InteractionRepository interactions = odb.interactions();
    if(interactions.size() > 0) {
        process_interactions (interactions, odb);
    }

    this->log_file->logVerbose("Reading odb constraints.");
    odb_ConstraintRepository constraints = odb.constraints();
    if(constraints.size() > 0) {
        process_constraints (constraints, odb);
    }

    odb_PartRepository& parts = odb.parts();
    odb_PartRepositoryIT parts_iter(parts);
    for (parts_iter.first(); !parts_iter.isDone(); parts_iter.next()) {
        this->log_file->logVerbose("Reading part: " + string(parts_iter.currentKey().CStr()));
        odb_Part part = parts[parts_iter.currentKey()];
        this->log_file->log("Part: " + string(part.name().CStr()));
        this->log_file->log("\tnode count: " + to_string(part.nodes().size()));
        this->log_file->log("\telement count: " + to_string(part.elements().size()));

        this->parts.push_back(process_part(part, odb));
        this->part_mesh[part.name().CStr()].part_index = this->parts.size() - 1;
    }

    this->log_file->logVerbose("Reading root assembly.");
    this->root_assembly = process_assembly(odb.rootAssembly(), odb);

}

section_category_type SpadeObject::process_section_category (const odb_SectionCategory &section_category) {
    section_category_type category;
    category.name = section_category.name().CStr();
    category.description = section_category.description().CStr();
    for (int i=0; i<section_category.sectionPoints().size(); i++) {
        odb_SectionPoint section_point = section_category.sectionPoints(i);
        category.section_point_numbers.push_back(to_string(section_point.number()));
        category.section_point_descriptions.push_back(section_point.description().CStr());
    }
    return category;
}

tangential_behavior_type SpadeObject::process_interaction_property (const odb_InteractionProperty &interaction_property) {
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
        this->log_file->logWarning("Unsupported Interaction Property Type");
    }
    return interaction;
}

nodes_type* SpadeObject::process_nodes (const odb_SequenceNode &nodes, const string &instance_name, const string &assembly_name, const string &set_name, const string &part_name, const int &embedded_space) {
    nodes_type new_nodes;
    string name;
    if (!part_name.empty()) { 
        try {  // If the node has been stored in nodes, just return the address to it
            new_nodes = this->part_mesh.at(part_name).nodes;  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
        } catch (const std::out_of_range& oor) {
            this->log_file->logDebug("New nodes for part: " + part_name);
            this->part_mesh[part_name].part_index = -1;
        }
    } else if (!assembly_name.empty()) {
        new_nodes = this->assembly_mesh[assembly_name].nodes;
    } else {
        name = instance_name;
        if (name.empty()) { 
            name = "ALL";
        }
        try {
            new_nodes = this->instance_mesh.at(name).nodes;  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
        } catch (const std::out_of_range& oor) {
            this->log_file->logDebug("New nodes for instance: " + name);
            this->instance_mesh[name].instance_index = -1;
        }
    }
    for (int i=0; i < nodes.size(); i++) { 
        odb_Node node = nodes.node(i);
        int node_label = node.label();
        node_type coords_sets;
        try {
            coords_sets = new_nodes.nodes.at(node_label);
            new_nodes.nodes[node_label].sets.insert(set_name);
            if (this->command_line_arguments->get("format") == "odb") {  // Don't need to store separate instances if using extract format
                if (!set_name.empty()) { new_nodes.node_sets[set_name].insert(node_label); }
            }
            continue;
        } catch (const std::out_of_range& oor) {
            if ((embedded_space == 2) || (embedded_space == 3)) {
                new_nodes.nodes[node_label].coordinates = {node.coordinates()[0], node.coordinates()[1]};
            } else {
                new_nodes.nodes[node_label].coordinates = {node.coordinates()[0], node.coordinates()[1], node.coordinates()[2]};
            }
            new_nodes.nodes[node_label].sets.insert(set_name);
            if (this->command_line_arguments->get("format") == "odb") {  // Don't need to store separate instances if using extract format
                if (!set_name.empty()) { new_nodes.node_sets[set_name].insert(node_label); }
            }
        }
    }
    if (!part_name.empty()) { 
            this->part_mesh[part_name].nodes = new_nodes;
            return &this->part_mesh[part_name].nodes;
    } else if (!assembly_name.empty()) {
            this->assembly_mesh[assembly_name].nodes = new_nodes;
            return &this->assembly_mesh[assembly_name].nodes;
    } else {
            this->instance_mesh[name].nodes = new_nodes;
            return &this->instance_mesh[name].nodes;
    }
}

elements_type* SpadeObject::process_elements (const odb_SequenceElement &elements, const string &instance_name, const string &assembly_name, const string &set_name, const string &part_name) {
    elements_type new_elements;
    string name;
    this->log_file->logDebug("\t\tCall to process_elements at time: " + this->command_line_arguments->getTimeStamp(true));
    if (!part_name.empty()) { 
        try {  // If the element has been stored in elements, just return the address to it
            new_elements = this->part_mesh.at(part_name).elements;  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
        } catch (const std::out_of_range& oor) {
            this->log_file->logDebug("New elements for part: " + part_name);
            this->part_mesh[part_name].part_index = -1;
            new_elements = this->part_mesh[part_name].elements;
        }
    } else if (!assembly_name.empty()) {
        new_elements = this->assembly_mesh[part_name].elements;
    } else {
        name = instance_name;
        if (name.empty()) { 
            name = "ALL";
        }
        try {
            new_elements = this->instance_mesh.at(name).elements;  // Use 'at' member function instead of brackets to get exception raised instead of creating blank value for key in map
        } catch (const std::out_of_range& oor) {
            this->log_file->logDebug("New elements for instance: " + name);
            this->instance_mesh[name].instance_index = -1;
            new_elements = this->instance_mesh[name].elements;
        }
    }
    this->log_file->logDebug("\t\tElements map retrieved in process_elements at time: " + this->command_line_arguments->getTimeStamp(true));
    int previous_label = -2;
    for (int i=0; i < elements.size(); i++) { 
        odb_Element element = elements.element(i);
        int element_label = element.label();
        if (element_label == previous_label) {  // For a typical hex element we were processing it 6 times
            continue;
        } else {
            previous_label = element_label;    // This will help ensure it's processed once
        }
        string type = element.type().CStr();

        if (new_elements.elements[type][element_label].instanceNames.empty()) {
            odb_SequenceString instance_names = element.instanceNames();
            for (int j=0; j < instance_names.size(); j++) { new_elements.elements[type][element_label].instanceNames.push_back(instance_names[j].CStr()); }
        }
        if (new_elements.elements[type][element_label].connectivity.empty()) {
            int element_connectivity_size;
            const int* const connectivity = element.connectivity(element_connectivity_size);
            for (int j=0; j < element_connectivity_size; j++) { new_elements.elements[type][element_label].connectivity.push_back(connectivity[j]); }
            this->log_file->logDebug("\t\tElement " + to_string(element_label) + ": connectivity count: " + to_string(new_elements.elements[type][element_label].connectivity.size()) + " instances count:" + to_string(new_elements.elements[type][element_label].instanceNames.size()) + " at time: " + this->command_line_arguments->getTimeStamp(true));
        }
        if (new_elements.elements[type][element_label].sectionCategory.name.empty()) {
            new_elements.elements[type][element_label].sectionCategory = process_section_category(element.sectionCategory());
        }
        new_elements.elements[type][element_label].sets.insert(set_name);
        if (!set_name.empty()) { new_elements.element_sets[set_name].insert(element_label); }
    }
    this->log_file->logDebug("\t\tFinished process_elements at time: " + this->command_line_arguments->getTimeStamp(true));
    if (!part_name.empty()) { 
            this->part_mesh[part_name].elements = new_elements;
            return &this->part_mesh[part_name].elements;
    } else if (!assembly_name.empty()) {
            this->assembly_mesh[assembly_name].elements = new_elements;
            return &this->assembly_mesh[assembly_name].elements;
    } else {
            this->instance_mesh[name].elements = new_elements;
            return &this->instance_mesh[name].elements;
    }

}

set_type SpadeObject::process_set(const odb_Set &odb_set) {
    set_type new_set;
    new_set.nodes = nullptr;
    new_set.elements = nullptr;
    if (odb_set.name().empty()) {
        return new_set;
    }

    new_set.name = odb_set.name().CStr();
    switch(odb_set.type()) {
        case odb_Enum::NODE_SET: new_set.type = "Node Set"; break;
        case odb_Enum::ELEMENT_SET: new_set.type = "Element Set"; break;
        case odb_Enum::SURFACE_SET: new_set.type = "Surface Set"; break;
    }
    this->log_file->logVerbose("\t\t" + new_set.name + ": " + new_set.type);

    odb_SequenceString names = odb_set.instanceNames();
    for (int i=0; i<names.size(); i++) {
        odb_String name = names.constGet(i);
        string instance_name = name.CStr();
        this->log_file->logDebug("\t\t\tProcessing set " + new_set.name + " of type " + new_set.type + " for" + instance_name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        if (!instance_name.empty()) { new_set.instanceNames.push_back(instance_name); }
        if (new_set.type == "Node Set") {
            const odb_SequenceNode& set_nodes = odb_set.nodes(name);
            if (!instance_name.empty()) {  // Don't process nodes or elements in set that doesn't belong to an instance
                new_set.nodes = process_nodes(set_nodes, instance_name, "", new_set.name, "", 0);
            }
        } else if (new_set.type == "Element Set") {
            const odb_SequenceElement& set_elements = odb_set.elements(name);
            if (!instance_name.empty()) {  // Don't process nodes or elements in set that doesn't belong to an instance
                new_set.elements = process_elements(set_elements, instance_name, "", new_set.name, "");
            }
        } else if (new_set.type == "Surface Set") {
            const odb_SequenceElement& set_elements = odb_set.elements(name);
            const odb_SequenceElementFace& set_faces = odb_set.faces(name);
            const odb_SequenceNode& set_nodes = odb_set.nodes(name);

            if(set_elements.size() && set_faces.size())
            {
                for (int n=0; n<set_elements.size(); n++) {
                    new_set.faces.push_back(this->faces_enum_strings[set_faces.constGet(n)]);
                }
                if (!instance_name.empty()) {  // Don't process nodes or elements in set that doesn't belong to an instance
                    new_set.elements = process_elements(set_elements, instance_name, "", new_set.name, "");
                }
            } else if((set_elements.size()) && (!instance_name.empty())) {
                new_set.elements = process_elements(set_elements, instance_name, "", new_set.name, "");
            } else {
                if (!instance_name.empty()) {  // Don't process nodes or elements in set that doesn't belong to an instance
                    new_set.nodes = process_nodes(set_nodes, instance_name, "", new_set.name, "", 0);
                }
            }

        } else {
            this->log_file->logWarning("Unknown set type.");
        }
    }
    return new_set;
}

void SpadeObject::process_interactions (const odb_InteractionRepository &interactions, odb_Odb &odb) {
    odb_InteractionRepositoryIT interaction_iter(interactions);
    for (interaction_iter.first(); !interaction_iter.isDone(); interaction_iter.next()) {
        contact_standard_type contact_standard;
        contact_explicit_type contact_explicit;
        odb_Interaction interaction = interaction_iter.currentValue();
        if (odb_isA(odb_SurfaceToSurfaceContactStd,interaction_iter.currentValue())) {
            this->log_file->logVerbose("Standard Surface To Surface Contact Interaction.");
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
            contact_standard.interactionProperty = process_interaction_property(interaction_property);

            odb_Set main = sscs.master();
            contact_standard.main = process_set(main);
            odb_Set secondary = sscs.slave();
            contact_standard.secondary = process_set(secondary);
            odb_Set adjust = sscs.adjustSet();
            if(!adjust.name().empty()) {
                contact_standard.adjust = process_set(adjust);
            }

        } else if (odb_isA(odb_SurfaceToSurfaceContactStd,interaction_iter.currentValue())) {
            this->log_file->logVerbose("Explicit Surface To Surface Contact Interaction.");
    	    odb_SurfaceToSurfaceContactExp ssce = odb_dynamicCast(odb_SurfaceToSurfaceContactExp,interaction_iter.currentValue());
            contact_explicit.sliding = ssce.sliding().CStr();
            contact_explicit.mainNoThick = (ssce.masterNoThick()) ? "true" : "false";
            contact_explicit.secondaryNoThick = (ssce.slaveNoThick()) ? "true" : "false";
            contact_explicit.mechanicalConstraint = ssce.mechanicalConstraint().CStr();
            contact_explicit.weightingFactorType = ssce.weightingFactorType().CStr();
            contact_explicit.weightingFactor = ssce.weightingFactor();
            contact_explicit.createStepName = ssce.createStepName().CStr();

            odb_InteractionProperty interaction_property = odb.interactionProperties().constGet(ssce.interactionProperty());
            contact_explicit.interactionProperty = process_interaction_property(interaction_property);

            contact_explicit.useReverseDatumAxis = (ssce.useReverseDatumAxis()) ? "true" : "false";
            contact_explicit.contactControls = ssce.contactControls().CStr();

            odb_Set main = ssce.master();
            contact_explicit.main = process_set(main);
            odb_Set secondary = ssce.slave();
            contact_explicit.secondary = process_set(secondary);

        } else {
              this->log_file->logWarning("Unsupported Interaction Type.");
        }
        this->standard_interactions.push_back(contact_standard);
        this->explicit_interactions.push_back(contact_explicit);
    }
}

void SpadeObject::process_constraints (const odb_ConstraintRepository &constraints, odb_Odb &odb) {
    odb_ConstraintRepositoryIT constraint_iter(constraints);
    int constraint_number = 1;

    for (constraint_iter.first(); !constraint_iter.isDone(); constraint_iter.next()) {
        if (odb_isA(odb_Tie,constraint_iter.currentValue())) {
            this->log_file->logVerbose("Tie Constraint");
            odb_Tie tie = odb_dynamicCast(odb_Tie,constraint_iter.currentValue());
            tie_type processed_tie = process_tie(tie);
            this->constraints.ties.push_back(processed_tie);
        } else if (odb_isA(odb_DisplayBody,constraint_iter.currentValue())) {
            this->log_file->logVerbose("Display Body Constraint");
            odb_DisplayBody display_body = odb_dynamicCast(odb_DisplayBody,constraint_iter.currentValue());
            display_body_type processed_display_body = process_display_body(display_body);
            this->constraints.display_bodies.push_back(processed_display_body);
        } else if (odb_isA(odb_Coupling,constraint_iter.currentValue())) {
            this->log_file->logVerbose("Coupling Constraint");
            odb_Coupling coupling = odb_dynamicCast(odb_Coupling,constraint_iter.currentValue());
            coupling_type processed_coupling = process_coupling(coupling);
            this->constraints.couplings.push_back(processed_coupling);
        } else if (odb_isA(odb_MPC,constraint_iter.currentValue())) {
            this->log_file->logVerbose("MPC Constraint");
            odb_MPC mpc = odb_dynamicCast(odb_MPC,constraint_iter.currentValue());
            mpc_type processed_mpc = process_mpc(mpc);
            this->constraints.mpc.push_back(processed_mpc);
        } else if (odb_isA(odb_ShellSolidCoupling,constraint_iter.currentValue())) {
            this->log_file->logVerbose("Shell Solid Coupling Constraint");
            odb_ShellSolidCoupling shell_solid_coupling = odb_dynamicCast(odb_ShellSolidCoupling,constraint_iter.currentValue());
            shell_solid_coupling_type processed_shell_solid_coupling = process_shell_solid_coupling(shell_solid_coupling);
            this->constraints.shell_solid_couplings.push_back(processed_shell_solid_coupling);
        } else {
            string constraint_name = constraint_iter.currentKey().CStr();
            this->log_file->logWarning("Unsupported Constraint type for constraint: " + constraint_name);
        }
        constraint_number++;
    }

}

tie_type SpadeObject::process_tie (const odb_Tie &tie) {
    tie_type new_tie;
    if (tie.hasValue())
    {
        new_tie.main = process_set(tie.master());
        new_tie.secondary = process_set(tie.slave());
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
display_body_type SpadeObject::process_display_body (const odb_DisplayBody &display_body) {
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
coupling_type SpadeObject::process_coupling (const odb_Coupling &coupling) {
    coupling_type new_coupling;
    if (coupling.hasValue())
    {
        new_coupling.surface = process_set(coupling.surface());
        new_coupling.refPoint = process_set(coupling.refPoint());

        new_coupling.couplingType = coupling.couplingType().cStr();
        new_coupling.weightingMethod = coupling.weightingMethod().cStr();
        new_coupling.influenceRadius = coupling.influenceRadius();
        new_coupling.u1 = (coupling.u1()) ? "true" : "false";
        new_coupling.u2 = (coupling.u2()) ? "true" : "false";
        new_coupling.u3 = (coupling.u3()) ? "true" : "false";
        new_coupling.ur1 = (coupling.ur1()) ? "true" : "false";
        new_coupling.ur2 = (coupling.ur2()) ? "true" : "false";
        new_coupling.ur3 = (coupling.ur3()) ? "true" : "false";
        new_coupling.nodes = process_set(coupling.couplingNodes());
    }
    return new_coupling;
}
mpc_type SpadeObject::process_mpc (const odb_MPC &mpc) {
    mpc_type new_mpc;
    if (mpc.hasValue())
    {
        new_mpc.surface = process_set(mpc.surface());
        new_mpc.refPoint = process_set(mpc.refPoint());
        new_mpc.mpcType = mpc.mpcType().cStr();
        new_mpc.userMode = mpc.userMode().cStr();
        new_mpc.userType = mpc.userType();
    }
    return new_mpc;
}
shell_solid_coupling_type SpadeObject::process_shell_solid_coupling (const odb_ShellSolidCoupling &shell_solid_coupling) {
    shell_solid_coupling_type new_shell_solid_coupling;
	if (shell_solid_coupling.hasValue())
	{
        new_shell_solid_coupling.shellEdge = process_set(shell_solid_coupling.shellEdge());
        new_shell_solid_coupling.solidFace = process_set(shell_solid_coupling.solidFace());
        new_shell_solid_coupling.positionToleranceMethod = shell_solid_coupling.positionToleranceMethod().cStr();
        new_shell_solid_coupling.positionTolerance = shell_solid_coupling.positionTolerance();
        new_shell_solid_coupling.influenceDistanceMethod = shell_solid_coupling.influenceDistanceMethod().cStr();
        new_shell_solid_coupling.influenceDistance = shell_solid_coupling.influenceDistance();

	}
    return new_shell_solid_coupling;
}

part_type SpadeObject::process_part (const odb_Part &part, odb_Odb &odb) {
    part_type new_part;
    new_part.nodes = nullptr;  // Set pointers to a null value when data type is created so as to have a "value" to test against later
    new_part.elements = nullptr;
    new_part.name = part.name().CStr();
    new_part.embeddedSpace = this->dimension_enum_strings[part.embeddedSpace()];

    const odb_SequenceNode& nodes = part.nodes();
    new_part.nodes = process_nodes(nodes, "", "", "", new_part.name, part.embeddedSpace());
    odb_SequenceElement elements = part.elements();
    new_part.elements = process_elements(elements, "", "", "", new_part.name);

    odb_SetRepositoryIT node_iter(part.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_part.nodeSets.push_back(process_set(node_iter.currentValue()));
    }
    odb_SetRepositoryIT element_iter(part.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_part.elementSets.push_back(process_set(element_iter.currentValue()));
    }
    odb_SetRepositoryIT surface_iter(part.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_part.surfaces.push_back(process_set(surface_iter.currentValue()));
    }

    return new_part;
}

section_assignment_type SpadeObject::process_section_assignment (const odb_SectionAssignment &section_assignment) {
    section_assignment_type new_section_assignment;
    new_section_assignment.region = process_set(section_assignment.region());
    new_section_assignment.sectionName = section_assignment.sectionName().CStr();
    return new_section_assignment;
}

beam_orientation_type SpadeObject::process_beam_orientation (const odb_BeamOrientation &beam_orientation) {
    beam_orientation_type new_beam_orientation;
    switch(beam_orientation.method()) {
        case odb_Enum::N1_COSINES: new_beam_orientation.method = "N1 Cosines"; break;
        case odb_Enum::CSYS: new_beam_orientation.method = "Csys"; break;
        case odb_Enum::VECT: new_beam_orientation.method = "Vector"; break;
    }
    new_beam_orientation.region = process_set(beam_orientation.region());
    odb_SequenceFloat beam_vector = beam_orientation.vector();
    for (int i=0; i<beam_vector.size(); i++) {
        new_beam_orientation.beam_vector.push_back(beam_vector[i]);
    }
    return new_beam_orientation;
}

rebar_orientation_type SpadeObject::process_rebar_orientation (const odb_RebarOrientation &rebar_orientation) {
    rebar_orientation_type new_rebar_orientation;
    switch(rebar_orientation.axis()) {
        case odb_Enum::AXIS_1: new_rebar_orientation.axis = "Axis 1"; break;
        case odb_Enum::AXIS_2: new_rebar_orientation.axis = "Axis 2"; break;
        case odb_Enum::AXIS_3: new_rebar_orientation.axis = "Axis 3"; break;
    }
    new_rebar_orientation.angle = rebar_orientation.angle();
    new_rebar_orientation.region = process_set(rebar_orientation.region());
    new_rebar_orientation.csys = process_csys(rebar_orientation.csys());

    return new_rebar_orientation;
}

datum_csys_type SpadeObject::process_csys(const odb_DatumCsys &datum_csys) {
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

analytic_surface_segment_type SpadeObject::process_segment (const odb_AnalyticSurfaceSegment &segment) {
    analytic_surface_segment_type new_segment;
    new_segment.type = segment.type().CStr();
    /*
    switch(segment.type()) {  // Documentation says these enum types exist, but they are not found in odb_Enum.h and gives compiler error
        case odb_Enum::CIRCLE: new_segment.column_size = 2; break;
        case odb_Enum::PARABOLA: new_segment.column_size = 2; break;
        case odb_Enum::START: new_segment.column_size = 1; break;
        case odb_Enum::LINE: new_segment.column_size = 1; break;
    }
    */
    odb_SequenceSequenceFloat segment_data = segment.data();
    new_segment.column_size = 0;
    new_segment.row_size = segment_data.size();
    for (int i=0; i<segment_data.size(); i++) {
        odb_SequenceFloat segment_data_dimension1 = segment_data[i];
        new_segment.column_size = segment_data_dimension1.size();
        for (int j=0; j<segment_data_dimension1.size(); j++) {
            new_segment.data.push_back(segment_data_dimension1[j]);
        }
    }
    return new_segment;
}

analytic_surface_type SpadeObject::process_analytic_surface (const odb_AnalyticSurface &analytic_surface) {
    analytic_surface_type new_analytic_surface;
    new_analytic_surface.name = analytic_surface.name().CStr();
    new_analytic_surface.type = analytic_surface.type().CStr();
    new_analytic_surface.filletRadius = analytic_surface.filletRadius();
    const odb_SequenceAnalyticSurfaceSegment& segments = analytic_surface.segments();
    for (int i=0; i<segments.size(); i++)  { new_analytic_surface.segments.push_back(process_segment(segments[i])); }
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

rigid_body_type SpadeObject::process_rigid_body (const odb_RigidBody &rigid_body) {
    rigid_body_type new_rigid_body;
    new_rigid_body.position = rigid_body.position().CStr();
    new_rigid_body.isothermal = (rigid_body.isothermal()) ? "true" : "false";
    try { new_rigid_body.referenceNode = process_set(rigid_body.referenceNodeSet());
    } catch(odb_BaseException& e) { this->log_file->logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.elements = process_set(rigid_body.elements());
    } catch(odb_BaseException& e) { this->log_file->logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.tieNodes = process_set(rigid_body.tieNodes());
    } catch(odb_BaseException& e) { this->log_file->logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.pinNodes = process_set(rigid_body.pinNodes());
    } catch(odb_BaseException& e) { this->log_file->logWarning(e.UserReport().CStr()); }
    try { new_rigid_body.analyticSurface = process_analytic_surface(rigid_body.analyticSurface());
    } catch(odb_BaseException& e) { this->log_file->logWarning(e.UserReport().CStr()); }
    return new_rigid_body;
}

instance_type SpadeObject::process_instance (const odb_Instance &instance, odb_Odb &odb) {
    instance_type new_instance;
    new_instance.nodes = nullptr;  // Set pointers to a null value when data type is created so as to have a "value" to test against later
    new_instance.elements = nullptr;
    new_instance.name = instance.name().CStr();
    new_instance.embeddedSpace = this->dimension_enum_strings[instance.embeddedSpace()];

    this->log_file->log("Instance: " + new_instance.name);
    this->log_file->log("\tnode count: " + to_string(instance.nodes().size()));
    this->log_file->log("\telement count: " + to_string(instance.elements().size()));

    const odb_SequenceNode& nodes = instance.nodes();
    new_instance.nodes = process_nodes(nodes, new_instance.name, "", "", "", instance.embeddedSpace());
    odb_SequenceElement elements = instance.elements();
    new_instance.elements = process_elements(elements, new_instance.name, "", "", "");

    this->log_file->logVerbose("\tnodeSets:");
    odb_SetRepositoryIT node_iter(instance.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_instance.nodeSets.push_back(process_set(node_iter.currentValue()));
    }
    this->log_file->log("\tnodeSet count: " + to_string(new_instance.nodeSets.size()));
    this->log_file->logVerbose("\telementSets:");
    odb_SetRepositoryIT element_iter(instance.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_instance.elementSets.push_back(process_set(element_iter.currentValue()));
    }
    this->log_file->log("\telementSet count: " + to_string(new_instance.elementSets.size()));
    odb_SetRepositoryIT surface_iter(instance.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_instance.surfaces.push_back(process_set(surface_iter.currentValue()));
    }
    this->log_file->log("\tsurface count: " + to_string(new_instance.surfaces.size()));
    const odb_SequenceRigidBody& rigid_bodies = instance.rigidBodies();
    for (int i=0; i<rigid_bodies.size(); i++)  { new_instance.rigidBodies.push_back(process_rigid_body(rigid_bodies[i])); }
    const odb_SequenceSectionAssignment& section_assignments = instance.sectionAssignments();
    for (int i=0; i<section_assignments.size(); i++)  { new_instance.sectionAssignments.push_back(process_section_assignment(section_assignments[i])); }
    const odb_SequenceBeamOrientation& beam_orientations = instance.beamOrientations();
    for (int i=0; i<beam_orientations.size(); i++)  { new_instance.beamOrientations.push_back(process_beam_orientation(beam_orientations[i])); }
    const odb_SequenceRebarOrientation& rebar_orientations = instance.rebarOrientations();
    for (int i=0; i<rebar_orientations.size(); i++)  { new_instance.rebarOrientations.push_back(process_rebar_orientation(rebar_orientations[i])); }
    try {
        new_instance.analyticSurface = process_analytic_surface(instance.analyticSurface());
    } catch(odb_BaseException& e) {
        this->log_file->logWarning(e.UserReport().CStr());
    }
    return new_instance;
}

connector_orientation_type SpadeObject::process_connector_orientation (const odb_ConnectorOrientation &connector_orientation) {
    connector_orientation_type new_connector_orientation;
    new_connector_orientation.region = process_set(connector_orientation.region());
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
    new_connector_orientation.localCsys1 = process_csys(connector_orientation.csys1());
    new_connector_orientation.localCsys2 = process_csys(connector_orientation.csys2());
    return new_connector_orientation;
}

assembly_type SpadeObject::process_assembly (odb_Assembly &assembly, odb_Odb &odb) {
    assembly_type new_assembly;
    new_assembly.nodes = nullptr;  // Set pointers to a null value when data type is created so as to have a "value" to test against later
    new_assembly.elements = nullptr;
    new_assembly.name = assembly.name().CStr();
    this->log_file->logVerbose("Reading assembly data for " + new_assembly.name);
    new_assembly.embeddedSpace = this->dimension_enum_strings[assembly.embeddedSpace()];

    this->log_file->log("Assembly: " + new_assembly.name);
    this->log_file->log("\tnode count: " + to_string(assembly.nodes().size()));
    this->log_file->log("\telement count: " + to_string(assembly.elements().size()));

    const odb_SequenceNode& nodes = assembly.nodes();
    string assembly_name = new_assembly.name;
    if (assembly_name.empty()) { assembly_name = "rootAssembly"; }
    new_assembly.nodes = process_nodes(nodes, "", assembly_name, "", "", assembly.embeddedSpace());
    const odb_SequenceElement& elements = assembly.elements();
    new_assembly.elements = process_elements(elements, "", new_assembly.name, "", "");

    this->log_file->logVerbose("\tnodeSets:");
    odb_SetRepositoryIT node_iter(assembly.nodeSets());
    for (node_iter.first(); !node_iter.isDone(); node_iter.next()) {
        new_assembly.nodeSets.push_back(process_set(node_iter.currentValue()));
    }
    this->log_file->log("\tnodeSet count: " + to_string(new_assembly.nodeSets.size()));
    this->log_file->logVerbose("\telementSets:");
    odb_SetRepositoryIT element_iter(assembly.elementSets());
    for (element_iter.first(); !element_iter.isDone(); element_iter.next()) {
        new_assembly.elementSets.push_back(process_set(element_iter.currentValue()));
    }
    this->log_file->log("\telementSet count: " + to_string(new_assembly.elementSets.size()));
    this->log_file->logVerbose("\tsurfaces:");
    odb_SetRepositoryIT surface_iter(assembly.surfaces());
    for (surface_iter.first(); !surface_iter.isDone(); surface_iter.next()) {
        new_assembly.surfaces.push_back(process_set(surface_iter.currentValue()));
    }
    this->log_file->log("\tsurface count: " + to_string(new_assembly.surfaces.size()));

    odb_InstanceRepository instances = assembly.instances();
    odb_InstanceRepositoryIT instance_iter(instances);
    for (instance_iter.first(); !instance_iter.isDone(); instance_iter.next()) {
        this->log_file->logVerbose("Reading instance: " + string(instance_iter.currentKey().CStr()));
        const odb_Instance& instance = instances[instance_iter.currentKey()];
        if ((this->command_line_arguments->get("instance") != "all") && (this->command_line_arguments->get("instance") != instance.name().CStr())) {
            continue;
        }

        new_assembly.instances.push_back(process_instance(instance, odb));
        this->instance_mesh[instance.name().CStr()].instance_index = new_assembly.instances.size() - 1;
    }
    odb_DatumCsysRepository datum_csyses = assembly.datumCsyses();
    odb_DatumCsysRepositoryIT datum_csyses_iter(datum_csyses);
    for (datum_csyses_iter.first(); !datum_csyses_iter.isDone(); datum_csyses_iter.next()) {
        const odb_DatumCsys& datum_csys = datum_csyses[datum_csyses_iter.currentKey()];
        new_assembly.datumCsyses.push_back(process_csys(datum_csys));
    }
    // TODO: Maybe reach out to 3DS to determine if they plan to implement 'odb_SequencePretensionSection' as it is declared, but not defined
//    const odb_SequencePretensionSection& pretension_sections = assembly.pretensionSections();
    const odb_SequenceConnectorOrientation& connector_orientations = assembly.connectorOrientations();
    for (int i=0; i<connector_orientations.size(); i++)  { new_assembly.connectorOrientations.push_back(process_connector_orientation(connector_orientations[i])); }
    return new_assembly;
}

void SpadeObject::process_field_values(const odb_FieldValue &field_value, const odb_SequenceInvariant& invariants, field_value_type &values) {
    int elementLabel = field_value.elementLabel();
    if (elementLabel != -1) { 
        values.elementEmpty = false;
    }
    values.elementLabel.push_back(elementLabel);
    int nodeLabel = field_value.nodeLabel();
    if (nodeLabel != -1) { 
        values.nodeEmpty = false;
    }
    values.nodeLabel.push_back(nodeLabel);
    int integrationPoint = field_value.integrationPoint();
    if (integrationPoint != -1) { 
        values.integrationPointEmpty = false;
    }
    values.integrationPoint.push_back(integrationPoint); 
    string value_type = get_field_type_enum(field_value.type());
    values.type.push_back(value_type);
    if (value_type != "") { values.typeEmpty = false; }
    if ((invariants.isMember(odb_Enum::MAGNITUDE)) && (field_value.magnitude() != 0)) {
        values.magnitude.push_back(field_value.magnitude());
        values.magnitudeEmpty = false;
    } else { values.magnitude.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::TRESCA)) && (field_value.tresca() != 0)) {
        values.tresca.push_back(field_value.tresca());
        values.trescaEmpty = false;
    } else { values.tresca.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::PRESS)) && (field_value.press() != 0)) {
        values.press.push_back(field_value.press());
        values.pressEmpty = false;
    } else { values.press.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::INV3)) && (field_value.inv3() != 0)) {
        values.inv3.push_back(field_value.inv3());
        values.inv3Empty = false;
    } else { values.inv3.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::MAX_PRINCIPAL)) && (field_value.maxPrincipal() != 0)) {
        values.maxPrincipal.push_back(field_value.maxPrincipal());
        values.maxPrincipalEmpty = false;
    } else { values.maxPrincipal.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::MID_PRINCIPAL)) && (field_value.midPrincipal() != 0)) {
        values.midPrincipal.push_back(field_value.midPrincipal());
        values.midPrincipalEmpty = false;
    } else { values.midPrincipal.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::MIN_PRINCIPAL)) && (field_value.minPrincipal() != 0)) {
        values.minPrincipal.push_back(field_value.minPrincipal());
        values.minPrincipalEmpty = false;
    } else { values.minPrincipal.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::MAX_INPLANE_PRINCIPAL)) && (field_value.maxInPlanePrincipal() != 0)) {
        values.maxInPlanePrincipal.push_back(field_value.maxInPlanePrincipal());
        values.maxInPlanePrincipalEmpty = false;
    } else { values.maxInPlanePrincipal.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::MIN_INPLANE_PRINCIPAL)) && (field_value.minInPlanePrincipal() != 0)) {
        values.minInPlanePrincipal.push_back(field_value.minInPlanePrincipal());
        values.minInPlanePrincipalEmpty = false;
    } else { values.minInPlanePrincipal.push_back(NAN); }
    if ((invariants.isMember(odb_Enum::OUTOFPLANE_PRINCIPAL)) && (field_value.outOfPlanePrincipal() != 0)) {
        values.outOfPlanePrincipal.push_back(field_value.outOfPlanePrincipal());
        values.outOfPlanePrincipalEmpty = false;
    } else { values.outOfPlanePrincipal.push_back(NAN); }
    string section_point_number =  to_string(field_value.sectionPoint().number());
    if (section_point_number != "-1") { 
        values.sectionPointNumber.push_back(section_point_number); 
        values.sectionPointNumberEmpty = false; 
    } else { values.sectionPointNumber.push_back(""); }
    string section_point_description = "";
    section_point_description =  field_value.sectionPoint().description().CStr();
    if (section_point_description != "") { 
        values.sectionPointDescriptionEmpty = false;
    }
    values.sectionPointDescription.push_back(section_point_description); 
}

string SpadeObject::get_field_type_enum(odb_Enum::odb_DataTypeEnum type_enum) {
    string value_type = "";
    switch(type_enum) {
        case odb_Enum::SCALAR: value_type = "Scalar"; break;
        case odb_Enum::VECTOR: value_type = "Vector"; break;
        case odb_Enum::TENSOR_3D_FULL: value_type = "Tensor 3D Full"; break;
        case odb_Enum::TENSOR_3D_PLANAR: value_type = "Tensor 3D Planar"; break;
        case odb_Enum::TENSOR_3D_SURFACE: value_type = "Tensor 3D Surface"; break;
        case odb_Enum::TENSOR_2D_PLANAR: value_type = "Tensor 2D Planar"; break;
        case odb_Enum::TENSOR_2D_SURFACE: value_type = "Tensor 2D Surface"; break;
    }
    return value_type;
}

string SpadeObject::get_valid_invariant_enum(odb_Enum::odb_InvariantEnum invariant_enum) {
    string invariant = "";
    switch(invariant_enum) {
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
    return invariant;
}

string SpadeObject::get_position_enum(odb_Enum::odb_ResultPositionEnum position_enum) {
    string position = "";
    switch(position_enum) {
        case odb_Enum::NODAL: position = "Nodal"; break;
        case odb_Enum::ELEMENT_NODAL: position = "Element Nodal"; break;
        case odb_Enum::INTEGRATION_POINT: position = "Integration Point"; break;
        case odb_Enum::ELEMENT_FACE: position = "Element Face"; break;
        case odb_Enum::ELEMENT_FACE_INTEGRATION_POINT: position = "Element Face Integration Point"; break;
        case odb_Enum::WHOLE_ELEMENT: position = "Whole Element"; break;
        case odb_Enum::WHOLE_REGION: position = "Whole Region"; break;
        case odb_Enum::WHOLE_PART_INSTANCE: position = "Whole Part Instance"; break;
        case odb_Enum::WHOLE_MODEL: position = "Whole Model"; break;
    }
    return position;
}

frame_type SpadeObject::process_frame (const odb_Frame &frame) {
    frame_type new_frame;
    new_frame.skip = false;
    new_frame.description = frame.description().CStr();
    new_frame.loadCase = frame.loadCase().name().CStr();
    switch(frame.domain()) {
        case odb_Enum::TIME: new_frame.domain = "Time"; break;
        case odb_Enum::FREQUENCY: new_frame.domain = "Frequency"; break;
        case odb_Enum::MODAL: new_frame.domain = "Modal"; break;
    }
    new_frame.incrementNumber = frame.incrementNumber();
    if ((this->command_line_arguments->get("frame") != "all") && (this->command_line_arguments->get("frame") != to_string(new_frame.incrementNumber))) {
        new_frame.skip = true;
        return new_frame;
    }
    new_frame.cyclicModeNumber = frame.cyclicModeNumber();
    new_frame.mode = frame.mode();
    new_frame.frameValue = frame.frameValue();
    new_frame.frequency = frame.frequency();
    if (this->command_line_arguments->get("frame-value") != "all") {
        if (to_string(new_frame.frameValue).find(this->command_line_arguments->get("frame-value")) == std::string::npos) {
            new_frame.skip = true;
            return new_frame;
        }
    }

    return new_frame;
}

history_point_type SpadeObject::process_history_point (const odb_HistoryPoint history_point) {
    history_point_type new_history_point;
    new_history_point.instanceName = history_point.instance().name().CStr();
    new_history_point.assemblyName = history_point.assembly().name().CStr();
    try {
        new_history_point.element_label = history_point.element().label();
        new_history_point.elementType = history_point.element().type().CStr();
        odb_SequenceString instance_names = history_point.element().instanceNames();
        for (int i=0; i < instance_names.size(); i++) { new_history_point.element.instanceNames.push_back(instance_names[i].CStr()); }
        int element_connectivity_size;
        const int* const connectivity = history_point.element().connectivity(element_connectivity_size);
        for (int i=0; i < element_connectivity_size; i++) { new_history_point.element.connectivity.push_back(connectivity[i]); }
        new_history_point.element.sectionCategory = process_section_category(history_point.element().sectionCategory());
        new_history_point.hasElement = true;
    } catch(odb_BaseException& exc) { new_history_point.hasElement = false; }
    new_history_point.node_label = history_point.node().label();
    new_history_point.node_coordinates[0] = history_point.node().coordinates()[0];
    new_history_point.node_coordinates[1] = history_point.node().coordinates()[1];
    new_history_point.node_coordinates[2] = history_point.node().coordinates()[2];
    if (history_point.node().label() < 0) {
        new_history_point.hasNode = false;
    } else {
        new_history_point.hasNode = true;
    }
    new_history_point.ipNumber = history_point.ipNumber();
    new_history_point.sectionPoint.number = to_string(history_point.sectionPoint().number());
    new_history_point.sectionPoint.description = history_point.sectionPoint().description().CStr();
    new_history_point.region = process_set(history_point.region());
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
    new_history_point.position = get_position_enum(history_point.position());

    return new_history_point;
}

history_region_type SpadeObject::process_history_region(const odb_HistoryRegion &history_region) {
    history_region_type new_history_region;
    new_history_region.name = history_region.name().CStr();
    new_history_region.description = history_region.description().CStr();
    new_history_region.position = get_position_enum(history_region.position());
    new_history_region.loadCase = history_region.loadCase().name().CStr();
    new_history_region.point = process_history_point(history_region.historyPoint());
    return new_history_region;
}

step_type SpadeObject::process_step(const odb_Step &step, odb_Odb &odb) {
    step_type new_step;
    new_step.name = step.name().CStr();
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
    return new_step;
}

void SpadeObject::write_step_data_h5 (odb_Odb &odb, H5::H5File &h5_file) {

    this->log_file->logVerbose("Reading steps.");
    odb_StepRepository step_repository = odb.steps();
    odb_StepRepositoryIT step_iter (step_repository);
    string steps_group_name = "/odb/steps";
    H5::Group steps_group = create_group(h5_file, steps_group_name);
    for (step_iter.first(); !step_iter.isDone(); step_iter.next())
    {
        // Read and write step data
        const odb_Step& current_step = step_repository[step_iter.currentKey()];
        if ((this->command_line_arguments->get("step") != "all") && (this->command_line_arguments->get("step") != current_step.name().CStr())) {
            continue;
        }
        step_type new_step = process_step(current_step, odb);  // Process and write step data that isn't history or field output

        string step_group_name = steps_group_name + "/" + replace_slashes(new_step.name);
        H5::Group step_group = create_group(h5_file, step_group_name);
        this->log_file->logVerbose("Writing top level step data for " + new_step.name);
        write_step(h5_file, step_group, new_step);

        // Read and write history output data
        write_history_data_h5 (odb, h5_file, current_step, step_group_name);
        // Read and write field output data
        write_frame_data_h5 (odb, h5_file, current_step, step_group_name);
    }
}

void SpadeObject::write_history_data_h5 (odb_Odb &odb, H5::H5File &h5_file, const odb_Step &step, const string &group_name) {
    const odb_HistoryRegionRepository& history_regions = step.historyRegions();
    odb_HistoryRegionRepositoryIT history_region_iterator (history_regions);

    string history_regions_group_name = group_name + "/historyRegions";
    H5::Group history_regions_group = create_group(h5_file, history_regions_group_name);
    for (history_region_iterator.first(); !history_region_iterator.isDone(); history_region_iterator.next())
    {
        const odb_HistoryRegion& history_region = history_region_iterator.currentValue();
        string history_region_name = history_region.name().CStr();
        if ((this->command_line_arguments->get("history-region") == "all") || (this->command_line_arguments->get("history-region") == history_region_name)) {
            this->log_file->logVerbose("Reading data for history region " + history_region_name);
            history_region_type new_history_region = process_history_region(history_region);
            string history_outputs_group_name;
            string history_output_group_name;
            this->log_file->logVerbose("Writing data for history region " + history_region_name);
            string history_region_group_name = history_regions_group_name + "/" + replace_slashes(history_region_name);
            write_history_region(h5_file, history_region_group_name, new_history_region);
            if (this->command_line_arguments->get("format") == "odb") {
                history_outputs_group_name = history_region_group_name + "/HistoryOutputs";
                H5::Group history_outputs_group = create_group(h5_file, history_outputs_group_name);
            } else if (this->command_line_arguments->get("format") == "extract") {
                history_outputs_group_name = step.name().CStr();  // Initialize group name
                create_extract_history_group(h5_file, new_history_region, history_outputs_group_name);  // Modifies group name
            }

            const odb_HistoryOutputRepository& history_outputs = history_region.historyOutputs();
            odb_HistoryOutputRepositoryIT history_outputs_iterator (history_outputs);
            for (history_outputs_iterator.first(); !history_outputs_iterator.isDone(); history_outputs_iterator.next()) {
                odb_HistoryOutput history_output = history_outputs_iterator.currentValue();
                string history_output_name = history_output.name().CStr();
                this->log_file->logVerbose("Writing data for history output " + history_output_name);
                if ((this->command_line_arguments->get("history") == "all") || (this->command_line_arguments->get("history") == history_output_name)) {
                    history_output_group_name = history_outputs_group_name + "/" + replace_slashes(history_output_name);
                    write_history_output(h5_file, history_output_group_name, history_output);
                }
            }
        }
    }
}

void SpadeObject::write_frame_data_h5 (odb_Odb &odb, H5::H5File &h5_file, const odb_Step &step, const string &group_name) {
    // Read and write frame data
    string frames_group_name = group_name + "/frames";
    H5::Group frames_group = create_group(h5_file, frames_group_name);
    const odb_SequenceFrame& frames = step.frames();
    for (int f=0; f<frames.size(); f++) {
        const odb_Frame& frame = frames.constGet(f);
        string frame_number = to_string(frame.incrementNumber());
        frame_type new_frame = process_frame(frame);

        if (!new_frame.skip) {

            this->log_file->logVerbose("Writing frame " + frame_number + " data");
            string frame_group_name = frames_group_name + "/" + frame_number;
            H5::Group frame_group = create_group(h5_file, frame_group_name);
            write_frame(h5_file, frame_group, new_frame);

            new_frame.max_length = 0;
            new_frame.max_width = 0;
            this->log_file->logVerbose("Writing field outputs for " + new_frame.description + ".");
            if (this->command_line_arguments->get("format") == "odb") {
                write_field_outputs(h5_file, frame, frame_group_name, new_frame.max_width, new_frame.max_length);
            } else if (this->command_line_arguments->get("format") == "extract") {
                write_extract_field_outputs(h5_file, frame, step.name().CStr(), new_frame.max_width, new_frame.max_length);
            }
            write_string_attribute(frame_group, "max_width", to_string(new_frame.max_width));
            write_string_attribute(frame_group, "max_length", to_string(new_frame.max_length));
        }
    }

}

void SpadeObject::write_h5_without_steps (H5::H5File &h5_file) {
// Write out data to hdf5 file


    this->log_file->logVerbose("Writing top level data to odb group.");
    H5::Group odb_group = create_group(h5_file, "/odb");
    write_string_attribute(odb_group, "name", this->name);
    write_string_attribute(odb_group, "analysisTitle", this->analysisTitle);
    write_string_attribute(odb_group, "description", this->description);
    write_string_attribute(odb_group, "path", this->path);
    write_string_attribute(odb_group, "isReadOnly", this->isReadOnly);

    this->log_file->logVerbose("Writing jobData.");
    H5::Group job_data_group = create_group(h5_file, "/odb/jobData");
    write_string_attribute(job_data_group, "analysisCode", this->job_data.analysisCode);
    write_string_attribute(job_data_group, "creationTime", this->job_data.creationTime);
    write_string_attribute(job_data_group, "machineName", this->job_data.machineName);
    write_string_attribute(job_data_group, "modificationTime", this->job_data.modificationTime);
    write_string_attribute(job_data_group, "name", this->job_data.name);
    write_string_attribute(job_data_group, "precision", this->job_data.precision);
    write_vector_attribute(job_data_group, "productAddOns", this->job_data.productAddOns);
    write_string_attribute(job_data_group, "version", this->job_data.version);

    if ((this->sector_definition.numSectors) || (!this->sector_definition.start_point.empty()) || (!this->sector_definition.end_point.empty())) {
        this->log_file->logVerbose("Writing sector definition at time: " + this->command_line_arguments->getTimeStamp(false));
        H5::Group sector_definition_group = create_group(h5_file, "/odb/sectorDefinition");
        write_integer_dataset(sector_definition_group, "numSectors", this->sector_definition.numSectors);
        if ((!this->sector_definition.start_point.empty()) || (!this->sector_definition.end_point.empty())) {
            H5::Group symmetry_axis_group = create_group(h5_file, "/odb/sectorDefinition/symmetryAxis");
            write_string_dataset(symmetry_axis_group, "StartPoint", this->sector_definition.start_point);
            write_string_dataset(symmetry_axis_group, "EndPoint", this->sector_definition.end_point);
        }
    }

    if (this->section_categories.size() > 0) {
        this->log_file->logVerbose("Writing section categories at time: " + this->command_line_arguments->getTimeStamp(false));
        H5::Group section_categories_group = create_group(h5_file, "/odb/sectionCategories");
        for (int i=0; i<this->section_categories.size(); i++) {
            string category_group_name = "/odb/sectionCategories/" + replace_slashes(this->section_categories[i].name);
            H5::Group section_category_group = create_group(h5_file, category_group_name);
            write_section_category(h5_file, section_category_group, category_group_name, this->section_categories[i]);
        }
    }

    if (this->user_xy_data.size() > 0) {
        this->log_file->logVerbose("Writing user data at time: " + this->command_line_arguments->getTimeStamp(false));
        H5::Group user_data_group = create_group(h5_file, "/odb/userData");
        for (int i=0; i<this->user_xy_data.size(); i++) {
            string user_xy_data_name = "/odb/userData/" + replace_slashes(this->user_xy_data[i].name);
            this->log_file->logVerbose("User data name:" + this->user_xy_data[i].name);
            H5::Group user_xy_data_group = create_group(h5_file, user_xy_data_name);
            write_string_dataset(user_xy_data_group, "sourceDescription", this->user_xy_data[i].sourceDescription);
            write_string_dataset(user_xy_data_group, "contentDescription", this->user_xy_data[i].contentDescription);
            write_string_dataset(user_xy_data_group, "positionDescription", this->user_xy_data[i].positionDescription);
            write_string_dataset(user_xy_data_group, "xAxisLabel", this->user_xy_data[i].xAxisLabel);
            write_string_dataset(user_xy_data_group, "yAxisLabel", this->user_xy_data[i].yAxisLabel);
            write_string_dataset(user_xy_data_group, "legendLabel", this->user_xy_data[i].legendLabel);
            write_string_dataset(user_xy_data_group, "description", this->user_xy_data[i].description);
            write_float_2D_data(user_xy_data_group, "data", this->user_xy_data[i].row_size, 2, this->user_xy_data[i].data);  // x-y data has two columns: x and y
        }
    }

    if ((!this->constraints.ties.empty()) || (!this->constraints.display_bodies.empty()) || (!this->constraints.couplings.empty()) || (!this->constraints.mpc.empty()) || (!this->constraints.shell_solid_couplings.empty())) {
        this->log_file->logVerbose("Writing constraints data at time: " + this->command_line_arguments->getTimeStamp(false));
        H5::Group contraints_group = create_group(h5_file, "/odb/constraints");
        write_constraints(h5_file, "odb/constraints");
    }
    this->log_file->logVerbose("Writing interactions data at time: " + this->command_line_arguments->getTimeStamp(false));
    write_interactions(h5_file, "odb");
    H5::Group parts_group = create_group(h5_file, "/odb/parts");
    this->log_file->logVerbose("Writing parts data at time: " + this->command_line_arguments->getTimeStamp(false));
    write_parts(h5_file, "odb/parts");
    this->log_file->logVerbose("Writing assembly data at time: " + this->command_line_arguments->getTimeStamp(false));
    write_assembly(h5_file, "odb/rootAssembly");
}

void SpadeObject::write_vtk_data (H5::H5File &h5_file) {

    // Specification at: https://docs.vtk.org/en/latest/design_documents/VTKFileFormats.html#vtkhdf-file-format
    this->log_file->logVerbose("Writing top level data to odb group.");
    H5::Group vtkhdf_group = create_group(h5_file, "/VTKHDF");
    write_string_attribute(vtkhdf_group, "Type", "UnstructuredGrid");  // Type can be: ImageData, PolyData, UnstructuredGrid, OverlappingAMR, PartitionedDataSetCollection or MultiBlockDataSet
    int version[2] = {2, 2};
    write_integer_array_attribute(vtkhdf_group, "Version", 2, version);

    // Create groups: PointData, CellData and FieldData
    H5::Group point_data_group = create_group(h5_file, "/VTKHDF/PointData");
    H5::Group cell_data_group = create_group(h5_file, "/VTKHDF/CellData");
    H5::Group field_data_group = create_group(h5_file, "/VTKHDF/FieldData");
}

H5::Group SpadeObject::open_subgroup(H5::H5File &h5_file, const string &sub_group_name, bool &exists) {
    try {
        exists = true;
        return h5_file.openGroup(sub_group_name.c_str());
    } catch(H5::Exception& e) {
        std::filesystem::path sub_group_path(sub_group_name);
        string parent_path = sub_group_path.parent_path().string();
        H5::Group parent_group = open_subgroup(h5_file, parent_path, exists);
    }
    exists = false;
    return create_group(h5_file, sub_group_name);
}


void SpadeObject::write_mesh(H5::H5File &h5_file) {
    this->log_file->logDebug("\tCalled write_mesh at time: " + this->command_line_arguments->getTimeStamp(true));
    string embedded_space;
    for (auto [part_name, part] : this->part_mesh) {
        embedded_space = "";
        string part_group_name = "/" + part_name;
        H5::Group extract_part_group = create_group(h5_file, part_group_name);
        if (!part.nodes.nodes.empty() || !part.elements.elements.empty()) {
            if (part.part_index >= 0) {
                if (!this->parts[part.part_index].embeddedSpace.empty()) {
                    embedded_space = this->parts[part.part_index].embeddedSpace;
                    write_string_dataset(extract_part_group, "embeddedSpace", embedded_space);
                }
            }
            string part_mesh_group_name = part_group_name + "/Mesh";
            H5::Group part_mesh_group = create_group(h5_file, part_mesh_group_name);
            if (!part.nodes.nodes.empty()) {
                write_mesh_nodes(h5_file, part_mesh_group, part.nodes.nodes, embedded_space);
            }
            if (!part.elements.elements.empty()) {
                write_mesh_elements(h5_file, part_mesh_group, part.elements.elements);
            }
        }
    }
    string assembly_group_name = "/" + this->root_assembly.name;
    H5::Group extract_assembly_group = create_group(h5_file, assembly_group_name);
    if (!this->root_assembly.nodes->nodes.empty() || !this->root_assembly.elements->elements.empty()) {
        if (!this->root_assembly.embeddedSpace.empty()) {
            write_string_dataset(extract_assembly_group, "embeddedSpace", this->root_assembly.embeddedSpace);
        }
        string assembly_mesh_group_name = assembly_group_name + "/Mesh";
        H5::Group assembly_mesh_group = create_group(h5_file, assembly_mesh_group_name);
        if (!this->root_assembly.nodes->nodes.empty()) {
            write_mesh_nodes(h5_file, assembly_mesh_group, this->root_assembly.nodes->nodes, this->root_assembly.embeddedSpace);
        }
        if (!this->root_assembly.elements->elements.empty()) {
            write_mesh_elements(h5_file, assembly_mesh_group, this->root_assembly.elements->elements);
        }
    }
    for (auto [instance_name, instance] : this->instance_mesh) {
        embedded_space = "";
        string instance_group_name = "/" + instance_name;
        H5::Group extract_instance_group = create_group(h5_file, instance_group_name);
        if (!instance.nodes.nodes.empty() || !instance.elements.elements.empty()) {
            if (instance.instance_index >= 0) {
                string instance_sub_group_name = instance_group_name + "/instance_data";
                H5::Group extract_instance_sub_group = create_group(h5_file, instance_sub_group_name);
                write_instance(h5_file, extract_instance_sub_group, instance_sub_group_name, this->root_assembly.instances[instance.instance_index]);
                if (!this->root_assembly.instances[instance.instance_index].embeddedSpace.empty()) { embedded_space = this->root_assembly.instances[instance.instance_index].embeddedSpace; }
            }
            string instance_mesh_group_name = instance_group_name + "/Mesh";
            H5::Group instance_mesh_group = create_group(h5_file, instance_mesh_group_name);
            if (!instance.nodes.nodes.empty()) {
                write_mesh_nodes(h5_file, instance_mesh_group, instance.nodes.nodes, embedded_space);
            }
            if (!instance.elements.elements.empty()) {
                write_mesh_elements(h5_file, instance_mesh_group, instance.elements.elements);
            }
        }
    }
    this->log_file->logDebug("\tFinished write_mesh at time: " + this->command_line_arguments->getTimeStamp(true));
}

void SpadeObject::write_mesh_nodes(H5::H5File &h5_file, H5::Group &group, map<int, node_type> nodes, const string embedded_space) {
    this->log_file->logDebug("\t\tCalled write_mesh_nodes at time: " + this->command_line_arguments->getTimeStamp(true));
    vector<string> coordinates;
    if (embedded_space == "Two Dimensional Planar") {
        coordinates = {"x", "y"};
    } else if (embedded_space == "AxiSymmetric") {
        coordinates = {"r", "z"};
    } else {  // Assume it's 3 dimensional with these names
        coordinates = {"x", "y", "z"};
    }
    int true_coord_size = nodes.begin()->second.coordinates.size();
    write_string_vector_dataset(group, "coordinates", coordinates);
    vector<int> node_labels;
    vector<float> node_coords;
    hsize_t dimension(nodes.size());
    vector<hvl_t> variable_length_sets(dimension);
    int node_count = 0;
    for (auto [node_id, node] : nodes) {
        node_labels.push_back(node_id);
        for (const float& coord : node.coordinates) {
            node_coords.push_back(coord);
        }
        variable_length_sets[node_count].len = node.sets.size();
        variable_length_sets[node_count].p = new char*[node.sets.size()];
        size_t set_count = 0;
        for (const std::string& str : node.sets) {
            char* c_str = new char[str.size() + 1];  // Plus 1 for null terminator
            std::strcpy(c_str, str.c_str());
            static_cast<char**>(variable_length_sets[node_count].p)[set_count] = c_str;
            ++set_count;
        }
        node_count++;
    }
    write_integer_vector_dataset(group, "node", node_labels);

    hsize_t dims[2] = {nodes.size(), true_coord_size};
    H5::DataSpace dataspace_coord(2, dims);
    H5::DataType datatype_coord(H5::PredType::NATIVE_FLOAT);

    try {
        H5::DataSet dataset_coord = group.createDataSet("node_location", datatype_coord, dataspace_coord);
        dataset_coord.write(node_coords.data(), datatype_coord);
        dataspace_coord.close();
        datatype_coord.close();
        dataset_coord.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset node_location. " + e.getDetailMsg());
        dataspace_coord.close();
        datatype_coord.close();
    }

    H5::DataSpace dataspace_sets(1, &dimension);
    H5::VarLenType datatype_sets(H5::StrType(0, H5T_VARIABLE));
    try {
        H5::DataSet dataset_sets(group.createDataSet("node_sets", datatype_sets, dataspace_sets));
        dataset_sets.write(variable_length_sets.data(), datatype_sets);
        dataspace_sets.close();
        datatype_sets.close();
        dataset_sets.close();
        // Clean up allocated memory
        H5Treclaim(datatype_sets.getId(), dataspace_sets.getId(), H5P_DEFAULT, variable_length_sets.data());
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset node_sets. " + e.getDetailMsg());
        dataspace_sets.close();
        datatype_sets.close();
    }
    // Clean up allocated memory
    for (size_t i = 0; i < variable_length_sets[0].len; ++i) {
         delete[] static_cast<char**>(variable_length_sets[0].p)[i];
    }
    H5free_memory(variable_length_sets[0].p);
    this->log_file->logDebug("\t\tFinished write_mesh_nodes at time: " + this->command_line_arguments->getTimeStamp(true));
}

void SpadeObject::write_mesh_elements(H5::H5File &h5_file, H5::Group &group, map<string, map<int, element_type>> elements) {
    this->log_file->logDebug("\t\tCalled write_mesh_elements at time: " + this->command_line_arguments->getTimeStamp(true));
    for (auto [type, element_members] : elements) {

        vector<int> element_labels;
        vector<string> section_categories_names;
        vector<string> section_categories_descriptions;
        vector<int> element_connectivity;
        int connectivity_size = element_members.begin()->second.connectivity.size();
        vector<int> node_indices;
        for (int i=0; i<connectivity_size; i++) { node_indices.push_back(i); }

        hsize_t dimension(element_members.size());
        vector<hvl_t> variable_length_sets(dimension);
        vector<hvl_t> variable_length_instances(dimension);
        vector<hvl_t> variable_length_section_point_numbers(dimension);
        vector<hvl_t> variable_length_section_point_descriptions(dimension);
        int element_count = 0;
        bool section_point_empty = true;
        for (auto [element_id, element] : element_members) {
            element_labels.push_back(element_id);
            section_categories_names.push_back(element.sectionCategory.name);
            section_categories_descriptions.push_back(element.sectionCategory.description);
            for (const float& connectivity : element.connectivity) {
                element_connectivity.push_back(connectivity);
            }
            variable_length_sets[element_count].len = element.sets.size();
            variable_length_sets[element_count].p = new char*[element.sets.size()];
            size_t set_count = 0;
            for (const std::string& str : element.sets) {
                char* c_str = new char[str.size() + 1];  // Plus 1 for null terminator
                std::strcpy(c_str, str.c_str());
                static_cast<char**>(variable_length_sets[element_count].p)[set_count] = c_str;
                ++set_count;
            }
            variable_length_instances[element_count].len = element.instanceNames.size();
            variable_length_instances[element_count].p = new char*[element.instanceNames.size()];
            size_t connectivity_count = 0;
            for (const std::string& str : element.instanceNames) {
                char* c_str = new char[str.size() + 1];  // Plus 1 for null terminator
                std::strcpy(c_str, str.c_str());
                static_cast<char**>(variable_length_instances[element_count].p)[connectivity_count] = c_str;
                ++connectivity_count;
            }
            if (element.sectionCategory.section_point_numbers.size()) {
                section_point_empty = false;
                variable_length_section_point_numbers[element_count].len = element.sectionCategory.section_point_numbers.size();
                variable_length_section_point_numbers[element_count].p = new char*[element.sectionCategory.section_point_numbers.size()];
                variable_length_section_point_descriptions[element_count].len = element.sectionCategory.section_point_descriptions.size();
                variable_length_section_point_descriptions[element_count].p = new char*[element.sectionCategory.section_point_descriptions.size()];
                size_t section_point_count = 0;
                for (int i=0; i<element.sectionCategory.section_point_numbers.size(); i++) {
                    string str_number = element.sectionCategory.section_point_numbers[i];
                    string str_description = element.sectionCategory.section_point_descriptions[i];
                    char* c_str_number = new char[str_number.size() + 1];  // Plus 1 for null terminator
                    char* c_str_description = new char[str_description.size() + 1];  // Plus 1 for null terminator
                    std::strcpy(c_str_number, str_number.c_str());
                    std::strcpy(c_str_description, str_description.c_str());
                    static_cast<char**>(variable_length_section_point_numbers[element_count].p)[section_point_count] = c_str_number;
                    static_cast<char**>(variable_length_section_point_descriptions[element_count].p)[section_point_count] = c_str_description;
                    ++section_point_count;
                }
            }
            element_count++;
        }
        write_integer_vector_dataset(group, type, element_labels);
        write_integer_vector_dataset(group, type + "_node", node_indices);
        write_string_vector_dataset(group, type + "_section_category_names", section_categories_names);
        write_string_vector_dataset(group, type + "_section_category_descriptions", section_categories_descriptions);

        hsize_t dims[2] = {element_members.size(), connectivity_size};
        H5::DataSpace dataspace_connectivity(2, dims);
        H5::DataType datatype_connectivity(H5::PredType::NATIVE_INT);
        try {
            H5::DataSet dataset_connectivity = group.createDataSet(type + "_mesh", datatype_connectivity, dataspace_connectivity);
            dataset_connectivity.write(element_connectivity.data(), datatype_connectivity);
            dataspace_connectivity.close();
            datatype_connectivity.close();
            dataset_connectivity.close();
        } catch(H5::Exception& e) {
            this->log_file->logWarning("Unable to create dataset " + type + "_mesh. " + e.getDetailMsg());
            dataspace_connectivity.close();
            datatype_connectivity.close();
        }

        H5::DataSpace dataspace_sets(1, &dimension);
        H5::VarLenType datatype_sets(H5::StrType(0, H5T_VARIABLE));
        try {
            H5::DataSet dataset_sets(group.createDataSet(type + "_element_sets", datatype_sets, dataspace_sets));
            dataset_sets.write(variable_length_sets.data(), datatype_sets);
            dataspace_sets.close();
            datatype_sets.close();
            dataset_sets.close();
            H5Treclaim(datatype_sets.getId(), dataspace_sets.getId(), H5P_DEFAULT, variable_length_sets.data());  // Clearing the memory
        } catch(H5::Exception& e) {
            this->log_file->logWarning("Unable to create dataset element_sets. " + e.getDetailMsg());
            dataspace_sets.close();
            datatype_sets.close();
        }

        H5::DataSpace dataspace_instances(1, &dimension);
        H5::VarLenType datatype_instances(H5::StrType(0, H5T_VARIABLE));
        try {
            H5::DataSet dataset_instances(group.createDataSet(type + "_element_instances", datatype_instances, dataspace_instances));
            dataset_instances.write(variable_length_instances.data(), datatype_instances);
            dataspace_instances.close();
            datatype_instances.close();
            dataset_instances.close();
            H5Treclaim(datatype_instances.getId(), dataspace_instances.getId(), H5P_DEFAULT, variable_length_instances.data());  // Clearing the memory
        } catch(H5::Exception& e) {
            this->log_file->logWarning("Unable to create dataset element_instances. " + e.getDetailMsg());
            dataspace_instances.close();
            datatype_instances.close();
        }

        if (!section_point_empty) {
            H5::DataSpace dataspace_section_point_numbers(1, &dimension);
            H5::VarLenType datatype_section_point_numbers(H5::StrType(0, H5T_VARIABLE));
            try {
                H5::DataSet dataset_section_point_numbers(group.createDataSet(type + "_section_point_numbers", datatype_section_point_numbers, dataspace_section_point_numbers));
                dataset_section_point_numbers.write(variable_length_section_point_numbers.data(), datatype_section_point_numbers);
                dataspace_section_point_numbers.close();
                datatype_section_point_numbers.close();
                dataset_section_point_numbers.close();
                H5Treclaim(datatype_section_point_numbers.getId(), dataspace_section_point_numbers.getId(), H5P_DEFAULT, variable_length_section_point_numbers.data());  // Clearing the memory
            } catch(H5::Exception& e) {
                this->log_file->logWarning("Unable to create dataset section_point_numbers. " + e.getDetailMsg());
                dataspace_section_point_numbers.close();
                datatype_section_point_numbers.close();
            }

            H5::DataSpace dataspace_section_point_descriptions(1, &dimension);
            H5::VarLenType datatype_section_point_descriptions(H5::StrType(0, H5T_VARIABLE));
            try {
                H5::DataSet dataset_section_point_descriptions(group.createDataSet(type + "_section_point_descriptions", datatype_section_point_descriptions, dataspace_section_point_descriptions));
                dataset_section_point_descriptions.write(variable_length_section_point_descriptions.data(), datatype_section_point_descriptions);
                dataspace_section_point_descriptions.close();
                datatype_section_point_descriptions.close();
                dataset_section_point_descriptions.close();
                H5Treclaim(datatype_section_point_descriptions.getId(), dataspace_section_point_descriptions.getId(), H5P_DEFAULT, variable_length_section_point_descriptions.data());  // Clearing the memory
            } catch(H5::Exception& e) {
                this->log_file->logWarning("Unable to create dataset section_point_descriptions. " + e.getDetailMsg());
                dataspace_section_point_descriptions.close();
                datatype_section_point_descriptions.close();
            }
        }

    }
    this->log_file->logDebug("\t\tFinished write_mesh_elements at time: " + this->command_line_arguments->getTimeStamp(true));
}

void SpadeObject::create_extract_history_group(H5::H5File &h5_file, history_region_type &history_region, string &step_group_name) {
    // First grab the name of the instance or assembly or simply use "ASSEMBLY"
    string instance_name = history_region.point.instanceName;
    if (instance_name.empty()) {
        instance_name = history_region.point.assemblyName;
        if (instance_name.empty()) { instance_name = this->default_instance_name; }
    }
    // Next open the instance->HistoryOutputs->region group, and create the parent groups if they don't exist
    string region_group_name = "/" + instance_name + "/HistoryOutputs/" + replace_slashes(history_region.name);
    bool sub_group_exists = true;
    H5::Group region_group = open_subgroup(h5_file, region_group_name, sub_group_exists);
    step_group_name = region_group_name + "/" + step_group_name;
    H5::Group step_group = create_group(h5_file, step_group_name);
}

void SpadeObject::write_parts(H5::H5File &h5_file, const string &group_name) {
    for (auto part : this->parts) {
        string part_group_name = group_name + "/" + replace_slashes(part.name);
        H5::Group part_group = create_group(h5_file, part_group_name);
        write_string_dataset(part_group, "embeddedSpace", part.embeddedSpace);
        if (this->command_line_arguments->get("format") == "odb") {
            write_nodes(h5_file, part_group, part_group_name, part.nodes, "");
            write_elements(h5_file, part_group, part_group_name, part.elements, "");
            write_sets(h5_file, part_group_name + "/nodeSets", part.nodeSets);
            write_sets(h5_file, part_group_name + "/elementSets", part.elementSets);
        }
        write_sets(h5_file, part_group_name + "/surfaces", part.surfaces);
    }
}

void SpadeObject::write_assembly(H5::H5File &h5_file, const string &group_name) {
    string root_assembly_group_name = "/odb/rootAssembly " + replace_slashes(this->root_assembly.name);
    H5::Group root_assembly_group = create_group(h5_file, root_assembly_group_name);
    this->log_file->logDebug("\tWriting instances in write_assembly at time: " + this->command_line_arguments->getTimeStamp(true));
    write_instances(h5_file, root_assembly_group_name);
    write_string_dataset(root_assembly_group, "embeddedSpace", this->root_assembly.embeddedSpace);
    if (this->command_line_arguments->get("format") == "odb") {
        write_nodes(h5_file, root_assembly_group, root_assembly_group_name, this->root_assembly.nodes, "");
        write_elements(h5_file, root_assembly_group, root_assembly_group_name, this->root_assembly.elements, "");
        write_sets(h5_file, root_assembly_group_name + "/nodeSets", this->root_assembly.nodeSets);
        write_sets(h5_file, root_assembly_group_name + "/elementSets", this->root_assembly.elementSets);
    }
    this->log_file->logDebug("\tWriting surface sets in write_assembly at time: " + this->command_line_arguments->getTimeStamp(true));
    write_sets(h5_file, root_assembly_group_name + "/surfaces", this->root_assembly.surfaces);
    if (this->root_assembly.connectorOrientations.size() > 0) {
        this->log_file->logDebug("\tWriting connector orientations in write_assembly at time: " + this->command_line_arguments->getTimeStamp(true));
        H5::Group connector_orientations_group = create_group(h5_file, root_assembly_group_name + "/connectorOrientations");
        for (int i=0; i<this->root_assembly.connectorOrientations.size(); i++) {
            string connector_orientation_group_name = root_assembly_group_name + "/connectorOrientations/" + to_string(i);
            H5::Group connector_orientation_group = create_group(h5_file, connector_orientation_group_name);
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
    this->log_file->logDebug("\tFinished write_assembly at time: " + this->command_line_arguments->getTimeStamp(true));
}

void SpadeObject::write_field_values(H5::H5File &h5_file, const string &group_name, H5::Group &group, field_value_type &values) {
    H5::Group value_group = create_group(h5_file, group_name);
    if (values.elementEmpty == false) { write_integer_vector_dataset(group, "elementLabel", values.elementLabel); }
    if (values.nodeEmpty == false) { write_integer_vector_dataset(group, "nodeLabel", values.nodeLabel); }
    if (values.integrationPointEmpty == false) { write_integer_vector_dataset(group, "integrationPoint", values.integrationPoint); }
    if (values.typeEmpty == false) { write_string_vector_dataset(group, "type", values.type); }
    if (values.magnitudeEmpty == false) { write_float_vector_dataset(group, "magnitude", values.magnitude); }
    if (values.trescaEmpty == false) { write_float_vector_dataset(group, "tresca", values.tresca); }
    if (values.pressEmpty == false) { write_float_vector_dataset(group, "press", values.press); }
    if (values.inv3Empty == false) { write_float_vector_dataset(group, "inv3", values.inv3); }
    if (values.maxPrincipalEmpty == false) { write_float_vector_dataset(group, "maxPrincipal", values.maxPrincipal); }
    if (values.midPrincipalEmpty == false) { write_float_vector_dataset(group, "midPrincipal", values.midPrincipal); }
    if (values.minPrincipalEmpty == false) { write_float_vector_dataset(group, "minPrincipal", values.minPrincipal); }
    if (values.maxInPlanePrincipalEmpty == false) { write_float_vector_dataset(group, "maxInPlanePrincipal", values.maxInPlanePrincipal); }
    if (values.minInPlanePrincipalEmpty == false) { write_float_vector_dataset(group, "minInPlanePrincipal", values.minInPlanePrincipal); }
    if (values.outOfPlanePrincipalEmpty == false) { write_float_vector_dataset(group, "outOfPlanePrincipal", values.outOfPlanePrincipal); }
    if ((values.sectionPointNumberEmpty == false) || (values.sectionPointDescriptionEmpty == false)) {
        H5::Group section_point_group = create_group(h5_file, group_name + "/sectionPoint");
        if (values.sectionPointNumberEmpty == false) {
            write_string_vector_dataset(section_point_group, "number", values.sectionPointNumber);
        }
        if (values.sectionPointDescriptionEmpty == false) {
            write_string_vector_dataset(section_point_group, "description", values.sectionPointDescription);
        }
    }
}

void SpadeObject::write_field_bulk_data(H5::H5File &h5_file, const string &group_name, const odb_FieldBulkData &field_bulk_data, bool complex_data, bool write_mises) {
    bool sub_group_exists = false;
    H5::Group bulk_group = open_subgroup(h5_file, group_name, sub_group_exists);

    int full_length = field_bulk_data.length() * field_bulk_data.width();
    int coord_length = 0;

    write_string_dataset(bulk_group, "position", get_position_enum(field_bulk_data.position()));
    if(field_bulk_data.numberOfElements() && field_bulk_data.elementLabels()) { // If elements

        int number_of_integration_points = field_bulk_data.length()/field_bulk_data.numberOfElements();
        coord_length = field_bulk_data.length() * field_bulk_data.orientationWidth();

        if (field_bulk_data.baseElementType().CStr() != "") {
            write_string_dataset(bulk_group, "baseElementType", field_bulk_data.baseElementType().CStr());
        }
        write_integer_dataset(bulk_group, "orientationWidth", field_bulk_data.orientationWidth());
        if (field_bulk_data.numberOfElements() != 0) {
            write_integer_dataset(bulk_group, "numberOfElements", field_bulk_data.numberOfElements());
        }
        if (field_bulk_data.valuesPerElement() != 0) {
            write_integer_dataset(bulk_group, "valuesPerElement", field_bulk_data.valuesPerElement());
        }

        vector<const char*> field_component_labels;
        odb_SequenceString component_labels = field_bulk_data.componentLabels();
        for (int i=0; i<field_bulk_data.componentLabels().size(); i++) {  // Usually just around 3 labels or less
            field_component_labels.push_back(component_labels[i].CStr());
        }
        write_c_string_vector_dataset(bulk_group, "componentLabels", field_component_labels);


        odb_Enum::odb_ElementFaceEnum* faces = field_bulk_data.faces();
        if (faces) {
            vector<const char*> faces_vector;
            int current_position = 0;
            for (int element=0; element<field_bulk_data.numberOfElements(); ++element) {
                for (int integration_point=0; integration_point<number_of_integration_points; integration_point++, current_position++) {
                    faces_vector.push_back(this->faces_enum_strings[faces[current_position]].c_str());
                }
            }
            write_c_string_2D_vector(bulk_group, "faces", number_of_integration_points, faces_vector);
        }

        if (write_mises) {
            write_float_2D_array(bulk_group, "mises", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.mises());
        }
        write_integer_2D_array(bulk_group, "elementLabels", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.elementLabels());
        if (field_bulk_data.integrationPoints()) {
            write_integer_2D_array(bulk_group, "integrationPoints", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.integrationPoints());
        }

        if(field_bulk_data.precision() == odb_Enum::SINGLE_PRECISION) {
            write_float_3D_array(bulk_group, "data", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.width(), field_bulk_data.data());
            if (complex_data) {
                write_float_3D_array(bulk_group, "conjugateData", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.width(), field_bulk_data.conjugateData());
            }
            if ((field_bulk_data.localCoordSystem()) && (coord_length)) {
                write_float_3D_array(bulk_group, "localCoordSystem", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.orientationWidth(), field_bulk_data.localCoordSystem());
            }
        } else {  // Double precision
            write_double_3D_array(bulk_group, "data", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.width(), field_bulk_data.dataDouble());
            if (complex_data) {
                write_double_3D_array(bulk_group, "conjugateData", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.width(), field_bulk_data.conjugateDataDouble());
            }
            if ((field_bulk_data.localCoordSystemDouble()) && (coord_length)) {
                write_double_3D_array(bulk_group, "localCoordSystem", field_bulk_data.numberOfElements(), number_of_integration_points, field_bulk_data.orientationWidth(), field_bulk_data.localCoordSystemDouble());
            }
        }
    } else {  // Nodes
        if(field_bulk_data.precision() == odb_Enum::SINGLE_PRECISION) {
            write_float_2D_array(bulk_group, "data", field_bulk_data.length(), field_bulk_data.width(), field_bulk_data.data());
            if (complex_data) {
                write_float_2D_array(bulk_group, "conjugateData", field_bulk_data.length(), field_bulk_data.width(), field_bulk_data.conjugateData());
            }
        } else {  // Double precision
            write_double_2D_array(bulk_group, "data", field_bulk_data.length(), field_bulk_data.width(), field_bulk_data.dataDouble());
            if (complex_data) {
                write_double_2D_array(bulk_group, "conjugateData", field_bulk_data.length(), field_bulk_data.width(), field_bulk_data.conjugateDataDouble());
            }
        }

        write_integer_array_dataset(bulk_group, "nodeLabels", field_bulk_data.length(), field_bulk_data.nodeLabels());
    }
}

void SpadeObject::write_field_outputs(H5::H5File &h5_file, const odb_Frame &frame, const string &group_name, int max_width, int max_length) {
    string field_outputs_group_name = group_name + "/fieldOutputs";
    H5::Group field_outputs_group = create_group(h5_file, field_outputs_group_name);

    const odb_FieldOutputRepository& field_outputs = frame.fieldOutputs();
    odb_FieldOutputRepositoryIT field_outputs_iterator(field_outputs);
    for (field_outputs_iterator.first(); !field_outputs_iterator.isDone(); field_outputs_iterator.next()) {
        const odb_FieldOutput& field_output = field_outputs[field_outputs_iterator.currentKey()];

        string field_output_name = field_output.name().CStr();
        string field_output_group_name = field_outputs_group_name + "/" + replace_slashes(field_output_name);
        H5::Group field_output_group = create_group(h5_file, field_output_group_name);
        this->log_file->logVerbose("Writing field output data for " + field_output_name);
        write_string_dataset(field_output_group, "name", field_output_name);
        write_string_dataset(field_output_group, "description", field_output.description().CStr());
        write_string_dataset(field_output_group, "type", get_field_type_enum(field_output.type()));
        write_integer_dataset(field_output_group, "dim", field_output.dim());
        write_integer_dataset(field_output_group, "dim2", field_output.dim2());
        // TODO: Maybe reach out to 3DS to determine if they plan to implement isEngineeringTensor() function
        // string isEngineeringTensor = (field_output.isEngineeringTensor()) ? "true" : "false";
        // write_string_dataset(field_output_group, "isEngineeringTensor", isEngineeringTensor);

        vector<const char*> component_labels;
        odb_SequenceString available_components = field_output.componentLabels();
        for (int i=0; i<available_components.size(); i++) {
            component_labels.push_back(available_components[i].CStr());
        }
        write_c_string_vector_dataset(field_output_group, "componentLabels", component_labels);

        vector<string> valid_invariants;
        for (int i=0; i<field_output.validInvariants().size(); i++) {
            valid_invariants.push_back(get_valid_invariant_enum(field_output.validInvariants().constGet(i)));
        }
        write_string_vector_dataset(field_output_group, "validInvariants", valid_invariants);

        odb_SequenceFieldLocation field_locations = field_output.locations();
        if (field_locations.size() > 0) {
            this->log_file->logVerbose("Writing field location data for " + field_output_name);
            H5::Group locations_group = create_group(h5_file, field_output_group_name + "/locations");
            for (int i=0; i<field_locations.size(); i++) {
                odb_FieldLocation field_location = field_locations.constGet(i);
                string location_group_name = field_output_group_name + "/locations/" + to_string(i);
                H5::Group location_group = create_group(h5_file, location_group_name);

                string position = get_position_enum(field_location.position());
                write_string_dataset(location_group, "position", position);
                if (field_location.sectionPoint().size() > 0) {
                    H5::Group section_points_group = create_group(h5_file, location_group_name + "/sectionPoint");
                    for (int j=0; j<field_location.sectionPoint().size(); j++) {
                        H5::Group section_point_group = create_group(h5_file, location_group_name + "/sectionPoint/" + to_string(field_location.sectionPoint(j).number()));
                        write_string_dataset(section_point_group, "description", field_location.sectionPoint(j).description().CStr());
                    }
                }
            }
        }


        map<string, field_value_type> values_map;  // String index is the name of the instance
        odb_SequenceFieldValue field_values = field_output.values();
        set<string> instance_names;
        bool write_mises = false;
        if (field_output.validInvariants().size() > 0) {
            field_value_type new_field_value;
            for (int i=0; i<field_values.size(); i++) {
                odb_FieldValue field_value  = field_values.constGet(i);
                string instance_name = field_value.instance().name().CStr();
                if (instance_name.empty()) { instance_name = this->default_instance_name; }
                if (instance_names.find(instance_name) == instance_names.end()) {
                    values_map[instance_name].magnitudeEmpty = true;
                    values_map[instance_name].trescaEmpty = true;
                    values_map[instance_name].pressEmpty = true;
                    values_map[instance_name].inv3Empty = true;
                    values_map[instance_name].maxPrincipalEmpty = true;
                    values_map[instance_name].midPrincipalEmpty = true;
                    values_map[instance_name].minPrincipalEmpty = true;
                    values_map[instance_name].maxInPlanePrincipalEmpty = true;
                    values_map[instance_name].minInPlanePrincipalEmpty = true;
                    values_map[instance_name].outOfPlanePrincipalEmpty = true;
                    values_map[instance_name].elementEmpty = true;
                    values_map[instance_name].nodeEmpty = true;
                    values_map[instance_name].integrationPointEmpty = true;
                    values_map[instance_name].typeEmpty = true;
                    values_map[instance_name].sectionPointNumberEmpty = true;
                    values_map[instance_name].sectionPointDescriptionEmpty = true;
                    instance_names.insert(instance_name);
                }
                process_field_values(field_value, field_output.validInvariants(), values_map[instance_name]);
            }
            if (field_output.validInvariants().isMember(odb_Enum::MISES)) {
                write_mises = true;
            }
        }

        int field_output_max_length = 0;
        int field_output_max_width = 0;
        set<string> field_data_names;
        const odb_SequenceFieldBulkData& field_bulk_values = field_output.bulkDataBlocks();
        this->log_file->logDebug("Writing " + to_string(field_bulk_values.size()) + " blocks of bulk field output data for " + field_output_name);
        for (int i=0; i<field_bulk_values.size(); i++) {  // There seems to be a "block" per element type and if the element type is the same per section point
                                                        // e.g. In one odb the "E" field values had three "blocks" One for element type B23 (section point 1)
                                                        // one for element type B23 (section point 5), and one for element type GAPUNI
            const odb_FieldBulkData& field_bulk_value = field_bulk_values[i];

            // Skip instance if 'all' not specified and instance doesn't match user input
            string instance_name = field_bulk_value.instance().name().CStr();
            bool instance_value_exists = false;
            if (instance_names.find(instance_name) != instance_names.end()) {
                instance_names.erase(instance_name);  // Remove from set to indicate it has already been processed
                instance_value_exists = true;
            }
            if (instance_name.empty()) { instance_name = this->default_instance_name; }
            if ((this->command_line_arguments->get("instance") != "all") && (this->command_line_arguments->get("instance") != instance_name)) {
                continue;
            }
            if (instance_value_exists) {
                string instance_group_name = field_output_group_name + "/" + instance_name;
                H5::Group instance_group = create_group(h5_file, instance_group_name);
                write_field_values(h5_file, instance_group_name, instance_group, values_map[instance_name]);
            }

            // Get name of group where to write data
            string position = get_position_enum(field_bulk_value.position());
            string data_name;
            string base_element_type = "";
            base_element_type = field_bulk_value.baseElementType().CStr();
            if (!base_element_type.empty()) { 
                data_name = field_bulk_value.baseElementType().CStr();
            } else {
                data_name = position;
            }
            if (field_data_names.find(data_name) != field_data_names.end()) {  // If this name already exists, append the index to it
                data_name = data_name + "_" + to_string(i);   // It would be nice to append the section point number to it, but I'm not sure how to do that reliably
            }
            field_data_names.insert(data_name);

            if (field_bulk_value.width() > field_output_max_width) {  field_output_max_width = field_bulk_value.width(); }
            if (field_bulk_value.length() > field_output_max_length) {  field_output_max_length = field_bulk_value.length(); }

            string value_group_name = field_output_group_name + "/" + instance_name + "/" + data_name;
            this->log_file->logDebug("Write field bulk data " + data_name);
            write_field_bulk_data(h5_file, value_group_name, field_bulk_value, field_output.isComplex(), write_mises);
        }

        if (field_output_max_width > max_width) {  max_width = field_output_max_width; }
        if (field_output_max_length > max_length) {  max_length = field_output_max_length; }

        for (const string& instance_name : instance_names) {
            if ((this->command_line_arguments->get("instance") != "all") && (this->command_line_arguments->get("instance") != instance_name)) {
                continue;
            }
            string instance_group_name = field_output_group_name + "/" + instance_name;
            H5::Group instance_group = create_group(h5_file, instance_group_name);
            write_field_values(h5_file, instance_group_name, instance_group, values_map[instance_name]);
        }
    }
}

void SpadeObject::write_extract_field_outputs(H5::H5File &h5_file, const odb_Frame &frame, const string &step_name, int max_width, int max_length) {
    string frame_number = to_string(frame.incrementNumber());

    const odb_FieldOutputRepository& field_outputs = frame.fieldOutputs();
    odb_FieldOutputRepositoryIT field_outputs_iterator(field_outputs);
    for (field_outputs_iterator.first(); !field_outputs_iterator.isDone(); field_outputs_iterator.next()) {
        const odb_FieldOutput& field_output = field_outputs[field_outputs_iterator.currentKey()];

        string field_output_name = field_output.name().CStr();
        string field_output_safe_name = replace_slashes(field_output_name);
        this->log_file->logVerbose("Writing field output data for " + field_output_name);

        vector<const char*> component_labels;
        odb_SequenceString available_components = field_output.componentLabels();
        for (int i=0; i<available_components.size(); i++) {
            component_labels.push_back(available_components[i].CStr());
        }

        vector<string> valid_invariants;
        for (int i=0; i<field_output.validInvariants().size(); i++) {
            valid_invariants.push_back(get_valid_invariant_enum(field_output.validInvariants().constGet(i)));
        }

        odb_SequenceFieldLocation field_locations = field_output.locations();

        map<string, field_value_type> values_map;  // String index is the name of the instance
        odb_SequenceFieldValue field_values = field_output.values();
        set<string> instance_names;
        bool write_mises = false;
        if (field_output.validInvariants().size() > 0) {
            field_value_type new_field_value;
            for (int i=0; i<field_values.size(); i++) {
                odb_FieldValue field_value  = field_values.constGet(i);
                string instance_name = field_value.instance().name().CStr();
                if (instance_name.empty()) { instance_name = this->default_instance_name; }
                if (instance_names.find(instance_name) == instance_names.end()) {
                    values_map[instance_name].magnitudeEmpty = true;
                    values_map[instance_name].trescaEmpty = true;
                    values_map[instance_name].pressEmpty = true;
                    values_map[instance_name].inv3Empty = true;
                    values_map[instance_name].maxPrincipalEmpty = true;
                    values_map[instance_name].midPrincipalEmpty = true;
                    values_map[instance_name].minPrincipalEmpty = true;
                    values_map[instance_name].maxInPlanePrincipalEmpty = true;
                    values_map[instance_name].minInPlanePrincipalEmpty = true;
                    values_map[instance_name].outOfPlanePrincipalEmpty = true;
                    values_map[instance_name].elementEmpty = true;
                    values_map[instance_name].nodeEmpty = true;
                    values_map[instance_name].integrationPointEmpty = true;
                    values_map[instance_name].typeEmpty = true;
                    values_map[instance_name].sectionPointNumberEmpty = true;
                    values_map[instance_name].sectionPointDescriptionEmpty = true;
                    instance_names.insert(instance_name);
                }
                process_field_values(field_value, field_output.validInvariants(), values_map[instance_name]);
            }
            if (field_output.validInvariants().isMember(odb_Enum::MISES)) {
                write_mises = true;
            }
        }

        int field_output_max_length = 0;
        int field_output_max_width = 0;
        set<string> field_data_names;
        const odb_SequenceFieldBulkData& field_bulk_values = field_output.bulkDataBlocks();
        this->log_file->logDebug("Writing " + to_string(field_bulk_values.size()) + " blocks of bulk field output data for " + field_output_name);
        for (int i=0; i<field_bulk_values.size(); i++) {  // There seems to be a "block" per element type and if the element type is the same per section point
                                                        // e.g. In one odb the "E" field values had three "blocks" One for element type B23 (section point 1)
                                                        // one for element type B23 (section point 5), and one for element type GAPUNI
            const odb_FieldBulkData& field_bulk_value = field_bulk_values[i];

            // Skip instance if 'all' not specified and instance doesn't match user input
            string instance_name = field_bulk_value.instance().name().CStr();
            bool instance_value_exists = false;
            if (instance_names.find(instance_name) != instance_names.end()) {
                instance_names.erase(instance_name);  // Remove from set to indicate it has already been processed
                instance_value_exists = true;
            }
            if (instance_name.empty()) { instance_name = this->default_instance_name; }
            if ((this->command_line_arguments->get("instance") != "all") && (this->command_line_arguments->get("instance") != instance_name)) {
                continue;
            }

            string field_output_group_name = instance_name + "/FieldOutputs/" + field_output_safe_name + "/" + step_name + "/" + frame_number;
            bool sub_group_exists = false;
            H5::Group field_output_group = open_subgroup(h5_file, field_output_group_name, sub_group_exists);
            if (instance_value_exists) {
                write_field_values(h5_file, field_output_group_name, field_output_group, values_map[instance_name]);
            }

            write_string_dataset(field_output_group, "name", field_output_name);
            write_string_dataset(field_output_group, "description", field_output.description().CStr());
            write_string_dataset(field_output_group, "type", get_field_type_enum(field_output.type()));
            write_integer_dataset(field_output_group, "dim", field_output.dim());
            write_integer_dataset(field_output_group, "dim2", field_output.dim2());
            // TODO: Maybe reach out to 3DS to determine if they plan to implement isEngineeringTensor() function
            // string isEngineeringTensor = (field_output.isEngineeringTensor()) ? "true" : "false";
            // write_string_dataset(field_output_group, "isEngineeringTensor", isEngineeringTensor);

            write_c_string_vector_dataset(field_output_group, "componentLabels", component_labels);
            write_string_vector_dataset(field_output_group, "validInvariants", valid_invariants);

            if (field_locations.size() > 0) {
                this->log_file->logDebug("Write field location data");
                H5::Group locations_group = create_group(h5_file, field_output_group_name + "/locations");
                for (int j=0; j<field_locations.size(); j++) {
                    odb_FieldLocation field_location = field_locations.constGet(j);
                    string location_group_name = field_output_group_name + "/locations/" + to_string(j);
                    H5::Group location_group = create_group(h5_file, location_group_name);

                    string position = get_position_enum(field_location.position());
                    write_string_dataset(location_group, "position", position);
                    if (field_location.sectionPoint().size() > 0) {
                        this->log_file->logDebug("Write section point data for location " + to_string(j));
                        H5::Group section_points_group = create_group(h5_file, location_group_name + "/sectionPoint");
                        for (int k=0; k<field_location.sectionPoint().size(); k++) {
                            H5::Group section_point_group = create_group(h5_file, location_group_name + "/sectionPoint/" + to_string(field_location.sectionPoint(k).number()));
                            write_string_dataset(section_point_group, "description", field_location.sectionPoint(k).description().CStr());
                        }
                    }
                }
            }

            // Get name of group where to write data
            string position = get_position_enum(field_bulk_value.position());
            string data_name;
            string base_element_type = "";
            base_element_type = field_bulk_value.baseElementType().CStr();
            if (!base_element_type.empty()) { 
                data_name = field_bulk_value.baseElementType().CStr();
            } else {
                data_name = position;
            }
            if (field_data_names.find(data_name) != field_data_names.end()) {  // If this name already exists, append the index to it
                data_name = data_name + "_" + to_string(i);   // It would be nice to append the section point number to it, but I'm not sure how to do that reliably
            }
            field_data_names.insert(data_name);

            if (field_bulk_value.width() > field_output_max_width) {  field_output_max_width = field_bulk_value.width(); }
            if (field_bulk_value.length() > field_output_max_length) {  field_output_max_length = field_bulk_value.length(); }

            string value_group_name = field_output_group_name + "/" + data_name;
            this->log_file->logDebug("Write field bulk data " + data_name);
            write_field_bulk_data(h5_file, value_group_name, field_bulk_value, field_output.isComplex(), write_mises);
        }

        if (field_output_max_width > max_width) {  max_width = field_output_max_width; }
        if (field_output_max_length > max_length) {  max_length = field_output_max_length; }

        // If there is an instance with field output data (not bulk data) that didn't get written, then write it here
        for (const string& instance_name : instance_names) {
            if ((this->command_line_arguments->get("instance") != "all") && (this->command_line_arguments->get("instance") != instance_name)) {
                continue;
            }
            string field_output_group_name = instance_name + "/FieldOutputs/" + field_output_safe_name + "/" + step_name + "/" + frame_number;
            bool sub_group_exists = false;
            H5::Group field_output_group = open_subgroup(h5_file, field_output_group_name, sub_group_exists);
            write_field_values(h5_file, field_output_group_name, field_output_group, values_map[instance_name]);

            write_string_dataset(field_output_group, "name", field_output_name);
            write_string_dataset(field_output_group, "description", field_output.description().CStr());
            write_string_dataset(field_output_group, "type", get_field_type_enum(field_output.type()));
            write_integer_dataset(field_output_group, "dim", field_output.dim());
            write_integer_dataset(field_output_group, "dim2", field_output.dim2());
            // TODO: Maybe reach out to 3DS to determine if they plan to implement isEngineeringTensor() function
            // string isEngineeringTensor = (field_output.isEngineeringTensor()) ? "true" : "false";
            // write_string_dataset(field_output_group, "isEngineeringTensor", isEngineeringTensor);

            write_c_string_vector_dataset(field_output_group, "componentLabels", component_labels);
            write_string_vector_dataset(field_output_group, "validInvariants", valid_invariants);

            if (field_locations.size() > 0) {
                H5::Group locations_group = create_group(h5_file, field_output_group_name + "/locations");
                for (int i=0; i<field_locations.size(); i++) {
                    odb_FieldLocation field_location = field_locations.constGet(i);
                    string location_group_name = field_output_group_name + "/locations/" + to_string(i);
                    H5::Group location_group = create_group(h5_file, location_group_name);

                    string position = get_position_enum(field_location.position());
                    write_string_dataset(location_group, "position", position);
                    if (field_location.sectionPoint().size() > 0) {
                        H5::Group section_points_group = create_group(h5_file, location_group_name + "/sectionPoint");
                        for (int j=0; j<field_location.sectionPoint().size(); j++) {
                            H5::Group section_point_group = create_group(h5_file, location_group_name + "/sectionPoint/" + to_string(field_location.sectionPoint(j).number()));
                            write_string_dataset(section_point_group, "description", field_location.sectionPoint(j).description().CStr());
                        }
                    }
                }
            }
        }
    }
}

void SpadeObject::write_frame(H5::H5File &h5_file, H5::Group &frame_group, frame_type &frame) {
    if (frame.cyclicModeNumber != -1) { write_integer_dataset(frame_group, "cyclicModeNumber", frame.cyclicModeNumber); }
    write_integer_dataset(frame_group, "mode", frame.mode);
    write_string_dataset(frame_group, "description", frame.description);
    write_string_dataset(frame_group, "domain", frame.domain);
    write_string_dataset(frame_group, "loadCase", frame.loadCase);
    write_float_dataset(frame_group, "frameValue", frame.frameValue);
    write_float_dataset(frame_group, "frequency", frame.frequency);
}

void SpadeObject::write_history_point(H5::H5File &h5_file, const string &group_name, history_point_type &history_point) {
    string history_point_group_name = group_name + "/point";
    H5::Group history_point_group = create_group(h5_file, history_point_group_name);
    if (this->command_line_arguments->get("format") == "odb") {
        write_string_dataset(history_point_group, "face", history_point.face);
        write_string_dataset(history_point_group, "position", history_point.position);
        write_string_dataset(history_point_group, "assembly", history_point.assemblyName);
        write_string_dataset(history_point_group, "instance", history_point.instanceName);
        write_integer_dataset(history_point_group, "ipNumber", history_point.ipNumber);
        if (history_point.hasElement) {
            string element_group_name = history_point_group_name + "/element";
            H5::Group element_group = create_group(h5_file, element_group_name);
            string element_label_group_name = history_point_group_name + "/element/" + to_string(history_point.element_label);
            H5::Group element_label_group = create_group(h5_file, element_label_group_name);
            write_string_dataset(element_label_group, "type", history_point.elementType);
            H5::Group section_category_group = create_group(h5_file, element_label_group_name + "/sectionCategory");
            write_section_category(h5_file, section_category_group, element_label_group_name + "/sectionCategory", history_point.element.sectionCategory);
            write_string_vector_dataset(element_label_group, "instanceNames", history_point.element.instanceNames);
            write_integer_vector_dataset(element_label_group, "connectivity", history_point.element.connectivity);
        }
        if (history_point.hasNode) {
            string node_group_name = history_point_group_name + "/node";
            H5::Group node_group = create_group(h5_file, node_group_name);
            write_float_array_dataset(node_group, to_string(history_point.node_label), 3, history_point.node_coordinates);
        }
        write_set(h5_file, history_point_group_name, history_point.region);
        H5::Group section_point_group = create_group(h5_file, history_point_group_name + "/sectionPoint");
        write_string_dataset(section_point_group, "number", history_point.sectionPoint.number);
        write_string_dataset(section_point_group, "description", history_point.sectionPoint.description);
    } else {  // Extract format - write less groups and data
        write_string_attribute(history_point_group, "face", history_point.face);
        write_string_attribute(history_point_group, "position", history_point.position);
        write_string_attribute(history_point_group, "assembly", history_point.assemblyName);
        write_string_attribute(history_point_group, "instance", history_point.instanceName);
        write_string_attribute(history_point_group, "ipNumber", to_string(history_point.ipNumber));
        if (history_point.hasElement) {   // Write just a dataset with the element type and label (e.g. element_CAX4T_1) and the element connectivity
            write_integer_vector_dataset(history_point_group, "element_" + history_point.elementType + "_" + to_string(history_point.element_label), history_point.element.connectivity);
        }
        if (history_point.hasNode) {  // Write a dataset with just the node label in the name and the node coordinates
            write_float_array_dataset(history_point_group, "node_" + to_string(history_point.node_label), 3, history_point.node_coordinates);
        }
        if (!history_point.region.name.empty()) {
            string set_group_name = history_point_group_name + "/" + replace_slashes(history_point.region.name);
            H5::Group set_group = create_group(h5_file, set_group_name);
            write_string_attribute(set_group, "type", history_point.region.type);
            write_string_vector_dataset(set_group, "instanceNames", history_point.region.instanceNames);
            if (!history_point.region.faces.empty()) {
                write_string_vector_dataset(set_group, "faces", history_point.region.faces);
            }
        }
        if (history_point.sectionPoint.number != "-1") { 
            write_string_attribute(history_point_group, "section_point_number", history_point.sectionPoint.number);
        }
        write_string_attribute(history_point_group, "section_point_description", history_point.sectionPoint.description);
    }

}

void SpadeObject::write_history_output(H5::H5File &h5_file, const string &group_name, const odb_HistoryOutput &history_output) {
    // Per Abaqus documentation the conjugate data specifies the imaginary portion of a specified complex variable at each 
    // frame value (time, frequency, or mode). Therefore it seems that data and conjugate data can be present at the same time
    // So a group has to be created two handle two possible datasets, despite there usually being only one
    H5::Group history_output_group = create_group(h5_file, group_name);

    write_string_dataset(history_output_group, "description", history_output.description().CStr());
    string history_output_type_name;
    switch(history_output.type()) {
        case odb_Enum::SCALAR: history_output_type_name = "Scalar"; break;
    }
    write_string_dataset(history_output_group, "type", history_output_type_name);

    vector<float> output_data;
    const odb_SequenceSequenceFloat& data = history_output.data();
    int row_size = data.size();
    for (int i=0; i<data.size(); i++) {
        odb_SequenceFloat data_dimension1 = data.constGet(i);
        for (int j=0; j<data_dimension1.size(); j++) {
            output_data.push_back(data_dimension1.constGet(j));
        }
    }
    this->log_file->logDebug("Writing " + to_string(row_size) + " rows of history output for " + history_output.name().CStr());
    write_float_2D_data(history_output_group, "data", row_size, 2, output_data);  // history output data has 2 columns: frameValue and value

    vector<float> output_conjugate_data;
    const odb_SequenceSequenceFloat& conjugate_data = history_output.conjugateData();
    int row_size_conjugate = conjugate_data.size();
    for (int i=0; i<conjugate_data.size(); i++) {
        odb_SequenceFloat conjugate_data_dimension1 = conjugate_data.constGet(i);
        for (int j=0; j<conjugate_data_dimension1.size(); j++) {
            output_conjugate_data.push_back(conjugate_data_dimension1.constGet(j));
        }
    }
    this->log_file->logDebug("Writing " + to_string(row_size_conjugate) + " rows of history output conjugate data for " + history_output.name().CStr());
    write_float_2D_data(history_output_group, "conjugateData", row_size_conjugate, 2, output_conjugate_data);
}

void SpadeObject::write_history_region(H5::H5File &h5_file, const string &group_name, history_region_type &history_region) {
    H5::Group history_region_group = create_group(h5_file, group_name);
    write_string_dataset(history_region_group, "description", history_region.description);
    write_string_dataset(history_region_group, "position", history_region.position);
    write_string_dataset(history_region_group, "loadCase", history_region.loadCase);
    write_history_point(h5_file, group_name, history_region.point);
}

void SpadeObject::write_step(H5::H5File &h5_file, H5::Group &step_group, step_type& step) {
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
}

void SpadeObject::write_instances(H5::H5File &h5_file, const string &group_name) {
    string instances_group_name = group_name + "/instances";
    H5::Group instances_group = create_group(h5_file, instances_group_name);
    for (auto instance : this->root_assembly.instances) {
        string instance_group_name = instances_group_name + "/" + replace_slashes(instance.name);
        H5::Group instance_group = create_group(h5_file, instance_group_name);
        this->log_file->logDebug("\t\tWriting instance: " + instance.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        write_instance(h5_file, instance_group, instance_group_name, instance);
    }
}

void SpadeObject::write_instance(H5::H5File &h5_file, H5::Group &group, const string &group_name, instance_type instance) {
    write_string_dataset(group, "embeddedSpace", instance.embeddedSpace);
    if (this->command_line_arguments->get("format") == "odb") {
        this->log_file->logDebug("\t\tWriting nodes in instance: " + instance.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        write_nodes(h5_file, group, group_name, instance.nodes, "");
        this->log_file->logDebug("\t\tWriting elements in instance: " + instance.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        write_elements(h5_file, group, group_name, instance.elements, "");
        this->log_file->logDebug("\t\tWriting node sets in instance: " + instance.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        write_sets(h5_file, group_name + "/nodeSets", instance.nodeSets);
        this->log_file->logDebug("\t\tWriting element sets in instance: " + instance.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
        write_sets(h5_file, group_name + "/elementSets", instance.elementSets);
    }
    write_sets(h5_file, group_name + "/surfaces", instance.surfaces);
    this->instance_links[instance.name] = group_name;
    if (instance.sectionAssignments.size() > 0) {
        H5::Group section_assignments_group = create_group(h5_file, group_name + "/sectionAssignments");
        for (int i=0; i<instance.sectionAssignments.size(); i++) {
            string section_assignment_group_name = group_name + "/sectionAssignments/" + instance.sectionAssignments[i].sectionName;
            H5::Group section_assignment_group = create_group(h5_file, section_assignment_group_name);
            write_set(h5_file, section_assignment_group_name, instance.sectionAssignments[i].region);
        }
    }
    if (instance.rigidBodies.size() > 0) {
        H5::Group rigid_bodies_group = create_group(h5_file, group_name + "/rigidBodies");
        for (int i=0; i<instance.rigidBodies.size(); i++) {
            string rigid_body_group_name = group_name + "/rigidBodies/" + to_string(i);
            H5::Group rigid_body_group = create_group(h5_file, rigid_body_group_name);
            write_string_dataset(rigid_body_group, "position", instance.rigidBodies[i].position);
            write_string_dataset(rigid_body_group, "isothermal", instance.rigidBodies[i].isothermal);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].referenceNode);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].elements);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].tieNodes);
            write_set(h5_file, rigid_body_group_name, instance.rigidBodies[i].pinNodes);
            write_analytic_surface(h5_file, rigid_body_group_name, instance.rigidBodies[i].analyticSurface);
        }
    }
    if (instance.beamOrientations.size() > 0) {
        H5::Group beam_orientations_group = create_group(h5_file, group_name + "/beamOrientations");
        for (int i=0; i<instance.beamOrientations.size(); i++) {
            string beam_orientation_group_name = group_name + "/beamOrientations/" + to_string(i);
            H5::Group beam_orientation_group = create_group(h5_file, beam_orientation_group_name);
            write_set(h5_file, beam_orientation_group_name, instance.beamOrientations[i].region);
            write_string_dataset(beam_orientation_group, "method", instance.beamOrientations[i].method);
            write_float_vector_dataset(beam_orientation_group, "vector", instance.beamOrientations[i].beam_vector);
        }
    }
    if (instance.rebarOrientations.size() > 0) {
        H5::Group rebar_orientations_group = create_group(h5_file, group_name  + "/rebarOrientations");
        for (int i=0; i<instance.rebarOrientations.size(); i++) {
            string rebar_orientation_group_name = group_name + "/rebarOrientations/" + to_string(i);
            H5::Group rebar_orientation_group = create_group(h5_file, rebar_orientation_group_name);
            write_string_dataset(rebar_orientation_group, "axis", instance.rebarOrientations[i].axis);
            write_float_dataset(rebar_orientation_group, "angle", instance.rebarOrientations[i].angle);
            write_set(h5_file, rebar_orientation_group_name, instance.rebarOrientations[i].region);
            write_datum_csys(h5_file, rebar_orientation_group_name, instance.rebarOrientations[i].csys);
        }
    }
    write_analytic_surface(h5_file, group_name, instance.analyticSurface);
}

void SpadeObject::write_analytic_surface(H5::H5File &h5_file, const string &group_name, analytic_surface_type &analytic_surface) {
    this->log_file->logDebug("\t\tCalled write_analytic_surface at time: " + this->command_line_arguments->getTimeStamp(true));
    if ((analytic_surface.name.empty()) && (analytic_surface.type.empty()) && (!analytic_surface.filletRadius) && (analytic_surface.segments.size() == 0) && (analytic_surface.localCoordData.size() == 0)) { return; }
    string analytic_surface_group_name = group_name + "/analyticSurface";
    H5::Group surface_group = create_group(h5_file, analytic_surface_group_name);
    write_string_dataset(surface_group, "name", analytic_surface.name);
    write_string_dataset(surface_group, "type", analytic_surface.type);
    write_double_dataset(surface_group, "filletRadius", analytic_surface.filletRadius);
    if (analytic_surface.segments.size() != 0) {
        H5::Group segments_group = create_group(h5_file, analytic_surface_group_name + "/segments");
        for (int i=0; i<analytic_surface.segments.size(); i++) {
            string segment_group_name = analytic_surface_group_name + "/segments/" + to_string(i);
            H5::Group segment_group = create_group(h5_file, segment_group_name);
            write_string_dataset(segment_group, "type", analytic_surface.segments[i].type);
            write_float_2D_data(segment_group, "data", analytic_surface.segments[i].row_size, analytic_surface.segments[i].column_size, analytic_surface.segments[i].data);
        }
    }
    write_float_2D_vector(surface_group, "localCoordData", analytic_surface.max_column_size, analytic_surface.localCoordData);
}

void SpadeObject::write_datum_csys(H5::H5File &h5_file, const string &group_name, const datum_csys_type &datum_csys) {
    string datum_group_name = group_name + "/csys";
    H5::Group datum_group = create_group(h5_file, datum_group_name);
    write_string_dataset(datum_group, "name", datum_csys.name);
    write_string_dataset(datum_group, "type", datum_csys.type);
    write_float_array_dataset(datum_group, "xAxis", 3, datum_csys.x_axis);
    write_float_array_dataset(datum_group, "yAxis", 3, datum_csys.y_axis);
    write_float_array_dataset(datum_group, "zAxis", 3, datum_csys.z_axis);
    write_float_array_dataset(datum_group, "origin", 3, datum_csys.origin);
}

void SpadeObject::write_constraints(H5::H5File &h5_file, const string &group_name) {
    if (!this->constraints.ties.empty()) {
        H5::Group ties_group = create_group(h5_file, group_name + "/tie");
        for (int i=0; i<this->constraints.ties.size(); i++) {
            string tie_group_name = group_name + "/tie/" + to_string(i);
            H5::Group tie_group = create_group(h5_file, tie_group_name);
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
        H5::Group display_bodies_group = create_group(h5_file, group_name + "/displayBody");
        for (int i=0; i<this->constraints.display_bodies.size(); i++) {
            string display_body_group_name = group_name + "/displayBody/" + to_string(i);
            H5::Group display_body_group = create_group(h5_file, display_body_group_name);
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
        H5::Group couplings_group = create_group(h5_file, group_name + "/coupling");
        for (int i=0; i<this->constraints.couplings.size(); i++) {
            string coupling_group_name = group_name + "/coupling/" + to_string(i);
            H5::Group coupling_group = create_group(h5_file, coupling_group_name);
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
        H5::Group mpcs_group = create_group(h5_file, group_name + "/mpc");
        for (int i=0; i<this->constraints.mpc.size(); i++) {
            string mpc_group_name = group_name + "/mpc/" + to_string(i);
            H5::Group mpc_group = create_group(h5_file, mpc_group_name);
            write_set(h5_file, mpc_group_name, this->constraints.mpc[i].surface);
            write_set(h5_file, mpc_group_name, this->constraints.mpc[i].refPoint);
            write_string_dataset(mpc_group, "mpcType", this->constraints.mpc[i].mpcType);
            write_string_dataset(mpc_group, "userMode", this->constraints.mpc[i].userMode);
            write_string_dataset(mpc_group, "userType", this->constraints.mpc[i].userType);
        }
    }
    if (!this->constraints.shell_solid_couplings.empty()) {
        H5::Group shell_solid_couplings_group = create_group(h5_file, group_name + "/shellSolidCoupling");
        for (int i=0; i<this->constraints.shell_solid_couplings.size(); i++) {
            string shell_solid_coupling_group_name = group_name + "/shellSolidCoupling/" + to_string(i);
            H5::Group shell_solid_coupling_group = create_group(h5_file, shell_solid_coupling_group_name);
            write_set(h5_file, shell_solid_coupling_group_name, this->constraints.shell_solid_couplings[i].shellEdge);
            write_set(h5_file, shell_solid_coupling_group_name, this->constraints.shell_solid_couplings[i].solidFace);
            write_string_dataset(shell_solid_coupling_group, "positionToleranceMethod", this->constraints.shell_solid_couplings[i].positionToleranceMethod);
            write_string_dataset(shell_solid_coupling_group, "positionTolerance", this->constraints.shell_solid_couplings[i].positionTolerance);
            write_string_dataset(shell_solid_coupling_group, "influenceDistanceMethod", this->constraints.shell_solid_couplings[i].influenceDistanceMethod);
            write_string_dataset(shell_solid_coupling_group, "influenceDistance", this->constraints.shell_solid_couplings[i].influenceDistance);
        }
    }
}

void SpadeObject::write_tangential_behavior(H5::H5File &h5_file, const string &group_name, tangential_behavior_type& tangential_behavior) {
    H5::Group tangential_behavior_group = create_group(h5_file, group_name + "/tangentialBehavior");
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
    H5::Group interactions_group = create_group(h5_file, group_name + "/interactions");
    if (!this->standard_interactions.empty()) {
        H5::Group interactions_group = create_group(h5_file, group_name + "/interactions/standard");
        for (int i=0; i<this->standard_interactions.size(); i++) {
            string standard_group_name = group_name + "/interactions/standard/" + to_string(i);
            H5::Group standards_group = create_group(h5_file, standard_group_name);
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
        H5::Group interactions_group = create_group(h5_file, group_name + "/interactions/explicit");
        for (int i=0; i<this->explicit_interactions.size(); i++) {
            string explicit_group_name = group_name + "/interactions/explicit/" + to_string(i);
            H5::Group explicit_group = create_group(h5_file, explicit_group_name);
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

void SpadeObject::write_elements(H5::H5File &h5_file, H5::Group &group, const string &group_name, const elements_type* elements, const string &set_name) {
    elements_type all_elements = *elements;
    this->log_file->logDebug("\t\tCall to write_elements at time: " + this->command_line_arguments->getTimeStamp(true));
    if (!all_elements.elements.empty()) {
        if (set_name.empty()) {
            H5::Group elements_group = create_group(h5_file, group_name + "/elements");
            for (auto [type, element] : all_elements.elements) {
                for (auto [element_id, element_members] : element) {
//                    this->log_file->logDebug("\t\t\tWrite element: " + to_string(element_id) + " of type: " + type + " at time: " + this->command_line_arguments->getTimeStamp(true));
                    string elements_label_group_name = group_name + "/elements/" + to_string(element_id);
                    H5::Group elements_label_group = create_group(h5_file, elements_label_group_name);
                    write_string_dataset(elements_label_group, "type", type);
                    H5::Group section_category_group = create_group(h5_file, elements_label_group_name + "/sectionCategory");
                    write_section_category(h5_file, section_category_group, elements_label_group_name + "/sectionCategory", element_members.sectionCategory);
                    write_string_vector_dataset(elements_label_group, "instanceNames", element_members.instanceNames);
                    write_integer_vector_dataset(elements_label_group, "connectivity", element_members.connectivity);
                }
            }
        } else {
            try {
                set<int> element_label_set = all_elements.element_sets.at(set_name);
                vector<int> element_labels(element_label_set.begin(), element_label_set.end());
                write_integer_vector_dataset(group, "elements", element_labels);
            } catch (const std::out_of_range& oor) {
                this->log_file->logDebug("Element set could not be found for : " + set_name);
            }
        }
    }
    this->log_file->logDebug("\t\tFinished write_elements at time: " + this->command_line_arguments->getTimeStamp(true));
}

void SpadeObject::write_nodes(H5::H5File &h5_file, H5::Group &group, const string &group_name, const nodes_type* nodes, const string &set_name) {
    nodes_type all_nodes = *nodes;
    if (!all_nodes.nodes.empty()) {
        if (set_name.empty()) {
            H5::Group nodes_group = create_group(h5_file, group_name + "/nodes");
            for (auto [node_id, node] : all_nodes.nodes) {
                write_float_vector_dataset(nodes_group, to_string(node_id), (node).coordinates);
            }
        } else {
            try {  // If the node has been stored in nodes, just return the address to it
                set<int> node_label_set = all_nodes.node_sets.at(set_name);
                vector<int> node_labels(node_label_set.begin(), node_label_set.end());
                write_integer_vector_dataset(group, "nodes", node_labels);
            } catch (const std::out_of_range& oor) {
                this->log_file->logDebug("Node set could not be found for : " + set_name);
            }
        }
    }
}

void SpadeObject::write_sets(H5::H5File &h5_file, const string &group_name, const vector<set_type> &sets) {
    if (!sets.empty()) {
        H5::Group sets_group = create_group(h5_file, group_name);
        for (auto odb_set : sets) { write_set(h5_file, group_name, odb_set); }
    }
}

void SpadeObject::write_set(H5::H5File &h5_file, const string &group_name, const set_type &odb_set) {
    std::regex nodes_pattern("\\s*ALL\\s*NODES\\s*");
    std::regex elements_pattern("\\s*ALL\\s*ELEMENTS\\s*");
    this->log_file->logDebug("\tWriting set " + odb_set.name + " at time: " + this->command_line_arguments->getTimeStamp(true));
    if ((!odb_set.name.empty()) && (!regex_match(odb_set.name, nodes_pattern)) && (!regex_match(odb_set.name, elements_pattern))) {
        // There is no reason to write a set named ' ALL NODES' when all the nodes can be found under the 'nodes' heading
        string set_group_name = group_name + "/" + replace_slashes(odb_set.name);
        H5::Group set_group = create_group(h5_file, set_group_name);
        write_string_attribute(set_group, "type", odb_set.type);
        write_string_vector_dataset(set_group, "instanceNames", odb_set.instanceNames);
        if (this->command_line_arguments->get("format") == "odb") {
            if (odb_set.type == "Node Set") {
                write_nodes(h5_file, set_group, set_group_name, odb_set.nodes, odb_set.name);
            } else if (odb_set.type == "Element Set") {
                write_elements(h5_file, set_group, set_group_name, odb_set.elements, odb_set.name);
            } else if (odb_set.type == "Surface Set") {
                if (!odb_set.faces.empty()) {
                    write_string_vector_dataset(set_group, "faces", odb_set.faces);
                }
                if (odb_set.elements != nullptr && !odb_set.elements->elements.empty()) {
                    write_elements(h5_file, set_group, set_group_name, odb_set.elements, odb_set.name);
                } else {
                    write_nodes(h5_file, set_group, set_group_name, odb_set.nodes, odb_set.name);
                }
            }
        } else {
            if (odb_set.type == "Surface Set") { 
                if (odb_set.elements != nullptr && !odb_set.elements->elements.empty()) {
                    write_elements(h5_file, set_group, set_group_name, odb_set.elements, odb_set.name);
                }
                if (!odb_set.faces.empty()) {
                    write_string_vector_dataset(set_group, "faces", odb_set.faces);
                }
            }
        }
    }
}

void SpadeObject::write_section_category(H5::H5File &h5_file, const H5::Group &group, const string &group_name, const section_category_type &section_category) {
    write_string_dataset(group, "description", section_category.description);
    for (int j=0; j<section_category.section_point_numbers.size(); j++) {
        string point_group_name = group_name + "/" + section_category.section_point_numbers[j];
        H5::Group section_point_group = create_group(h5_file, point_group_name);
        write_string_dataset(section_point_group, "description", section_category.section_point_descriptions[j]);
    }
}

void SpadeObject::write_string_attribute(const H5::Group &group, const string &attribute_name, const string &string_value) {
    if (string_value.empty()) { return; }
    H5::DataSpace attribute_space(H5S_SCALAR);
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);  // Use the length of the string or 1 if string is blank
    try {
        H5::Attribute attribute = group.createAttribute(attribute_name, string_type, attribute_space);
        attribute.write(string_type, string_value);
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create attribute " + attribute_name + ". " + e.getDetailMsg());
    }
    attribute_space.close();
}

void SpadeObject::write_integer_array_attribute(const H5::Group &group, const string &attribute_name, const int array_size, const int* int_array) {
    if (!int_array) { return; }
    hsize_t dimensions[1] = {array_size};
    H5::DataSpace attribute_space(1, dimensions);
    try {
        H5::Attribute attribute = group.createAttribute(attribute_name, H5::PredType::NATIVE_INT, attribute_space);
        attribute.write(H5::PredType::NATIVE_INT, int_array);
        attribute.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create attribute " + attribute_name + ". " + e.getDetailMsg());
    }
    attribute_space.close();
}

void SpadeObject::write_vector_attribute(const H5::Group &group, const string &attribute_name, const vector<string> &string_values) {
    if (string_values.empty()) { return; }
    // Convert the vector of strings to a C-style array of c-strings
    const char** c_string_array = new const char*[string_values.size()];
    for (size_t i = 0; i < string_values.size(); ++i) {
        c_string_array[i] = string_values[i].c_str();
    }
    hsize_t dimensions[1] {string_values.size()};
    H5::DataSpace  attribute_space(1, dimensions);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE); // Variable length string
    try {
        H5::Attribute attribute = group.createAttribute(attribute_name, string_type, attribute_space);
        attribute.write(string_type, c_string_array);
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create attribute " + attribute_name + ". " + e.getDetailMsg());
    }
    // Clean up
    delete[] c_string_array;
    attribute_space.close();
}

void SpadeObject::write_string_dataset(const H5::Group& group, const string & dataset_name, const string & string_value) {
    if (string_value.empty()) { return; }
    H5::DataSpace attribute_space(H5S_SCALAR);
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one string
    int string_size = string_value.size();
    if (string_size == 0) { string_size++; }  // If the string is empty, make the string size equal to one, as StrType must have a positive size
    H5::StrType string_type (0, string_size);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
        dataset.write(&string_value[0], string_type);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_string_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<string> & string_values) {
    if (string_values.empty()) { return; }
    std::vector<const char*> c_string_array;
    for (int i = 0; i < string_values.size(); ++i) {
        c_string_array.push_back(string_values[i].c_str());
    }
    hsize_t dimensions[1] {c_string_array.size()};
    H5::DataSpace  dataspace(1, dimensions);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE); // Variable length string
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
        dataset.write(c_string_array.data(), string_type);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_c_string_vector_dataset(const H5::Group& group, const string & dataset_name, vector<const char*> & string_values) {
    if (string_values.empty()) { return; }
    hsize_t dimensions[1] {string_values.size()};
    H5::DataSpace  dataspace(1, dimensions);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE); // Variable length string
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
        dataset.write(string_values.data(), string_type);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_c_string_2D_vector(const H5::Group& group, const string & dataset_name, const int & column_size, vector<const char*> & string_data) {
    if (string_data.empty()) { return; }
    hsize_t dimensions[] {string_data.size(), column_size};
    H5::DataSpace  dataspace(2, dimensions);
    H5::StrType string_type(H5::PredType::C_S1, H5T_VARIABLE); // Variable length string
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, string_type, dataspace);
        dataset.write(string_data.data(), string_type);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_integer_dataset(const H5::Group& group, const string & dataset_name, const int & int_value) {
    if (!int_value) { return; }
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        dataset.write(&int_value, H5::PredType::NATIVE_INT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_integer_array_dataset(const H5::Group& group, const string & dataset_name, const int array_size, const int* int_array) {
    if (!int_array) { return; }
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        dataset.write(int_array, H5::PredType::NATIVE_INT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_integer_vector_dataset(const H5::Group& group, const string & dataset_name, const vector<int> & int_data) {
    if (int_data.empty()) { return; }
    hsize_t dimensions[] = {int_data.size()};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        dataset.write(int_data.data(), H5::PredType::NATIVE_INT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_integer_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, int *integer_array) {
    if (!integer_array) { return; }
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_INT, dataspace);
        dataset.write(integer_array, H5::PredType::NATIVE_INT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_dataset(const H5::Group &group, const string &dataset_name, const float &float_value) {
//    if (!float_value) { return; }
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(&float_value, H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const float* float_array) {
    if (!float_array) { return; }
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<float> &float_data) {
    if (float_data.empty()) { return; }
    hsize_t dimensions[] = {float_data.size()};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_data.data(), H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, float *float_array) {
    if (!float_array) { return; }
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_3D_array(const H5::Group &group, const string &dataset_name, const int &aisle_size, const int &row_size, const int &column_size, float *float_array) {
    if (!float_array) { return; }
    hsize_t dimensions[] = {aisle_size, row_size, column_size};
    H5::DataSpace dataspace(3, dimensions);  // three dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_array, H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_2D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, const vector<float> &float_data) {
    if (float_data.empty()) { return; }
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_FLOAT, dataspace);
        dataset.write(float_data.data(), H5::PredType::NATIVE_FLOAT);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_float_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, vector<vector<float>> &float_data) {
    if (float_data.empty()) { return; }
    hsize_t dimensions(float_data.size());
    H5::DataSpace dataspace(1, &dimensions);
    H5::VarLenType datatype(H5::PredType::NATIVE_FLOAT);
    try {
        H5::DataSet dataset(group.createDataSet(dataset_name, datatype, dataspace));
        vector<hvl_t> variable_length(dimensions);
        for (hsize_t i = 0; i < dimensions; ++i)
        {
            variable_length[i].len = float_data[i].size();
            variable_length[i].p = &float_data[i][0];
        }
        dataset.write(variable_length.data(), datatype);
        dataspace.close();
        datatype.close();
        dataset.close();
        H5Treclaim(datatype.getId(), dataspace.getId(), H5P_DEFAULT, variable_length.data());  // Clearing the memory
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
        dataspace.close();
        datatype.close();
    }
}

void SpadeObject::write_double_dataset(const H5::Group &group, const string &dataset_name, const double &double_value) {
    if (!double_value) { return; }
    hsize_t dimensions[] = {1};
    H5::DataSpace dataspace(1, dimensions);  // Just one integer
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(&double_value, H5::PredType::NATIVE_DOUBLE);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_double_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const double* double_array) {
    if (!double_array) { return; }
    hsize_t dimensions[] = {array_size};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_array, H5::PredType::NATIVE_DOUBLE);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_double_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<double> &double_data) {
    if (double_data.empty()) { return; }
    hsize_t dimensions[] = {double_data.size()};
    H5::DataSpace dataspace(1, dimensions);
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_data.data(), H5::PredType::NATIVE_DOUBLE);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_double_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, double *double_array) {
    if (!double_array) { return; }
    hsize_t dimensions[] = {row_size, column_size};
    H5::DataSpace dataspace(2, dimensions);  // two dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_array, H5::PredType::NATIVE_DOUBLE);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_double_3D_array(const H5::Group &group, const string &dataset_name, const int &aisle_size, const int &row_size, const int &column_size, double *double_array) {
    if (!double_array) { return; }
    hsize_t dimensions[] = {aisle_size, row_size, column_size};
    H5::DataSpace dataspace(3, dimensions);  // three dimensional data
    try {
        H5::DataSet dataset = group.createDataSet(dataset_name, H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(double_array, H5::PredType::NATIVE_DOUBLE);
        dataset.close();
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
    }
    dataspace.close();
}

void SpadeObject::write_double_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, vector<vector<double>> & double_data) {
    if (double_data.empty()) { return; }
     // Convert to 2D array
    hsize_t dimensions(double_data.size());
    H5::DataSpace dataspace(1, &dimensions);
    H5::VarLenType datatype(H5::PredType::NATIVE_DOUBLE);
    try {
        H5::DataSet dataset(group.createDataSet(dataset_name, datatype, dataspace));
        vector<hvl_t> variable_length(dimensions);
        for (hsize_t i = 0; i < dimensions; ++i)
        {
            variable_length[i].len = double_data[i].size();
            variable_length[i].p = &double_data[i][0];
        }
        dataset.write(variable_length.data(), datatype);
        dataspace.close();
        datatype.close();
        dataset.close();
        H5Treclaim(datatype.getId(), dataspace.getId(), H5P_DEFAULT, variable_length.data());  // Clearing the memory
    } catch(H5::Exception& e) {
        this->log_file->logWarning("Unable to create dataset " + dataset_name + ". " + e.getDetailMsg());
        dataspace.close();
        datatype.close();
    }
}

H5::Group SpadeObject::create_group(H5::H5File &h5_file, const string &group_name) {
    H5::Exception::dontPrint();
    try {
        return h5_file.createGroup(group_name.c_str());
    } catch(H5::Exception& e) {
        try {
            return h5_file.openGroup(group_name.c_str());
        } catch(H5::Exception& e) {
            this->log_file->logErrorAndExit("Unable to create or open group " + group_name + ". " + e.getDetailMsg());
        }
    }

}

string SpadeObject::replace_slashes(const string &name) {
    string clean_name = name;
    std::replace(clean_name.begin(), clean_name.end(), '/', '|');   // Can't have a slash in a group name for hdf5 files
    return clean_name;
}


void SpadeObject::write_yaml_without_steps () {
    // TODO: write this function if requested
}

void SpadeObject::write_json_without_steps () {
    // TODO: write this function if requested
}
