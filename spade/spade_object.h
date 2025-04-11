//! An object for holding the data from the odb

#include <vector>
#include <array>

#include "H5Cpp.h"
using namespace H5;
#include <odb_API.h>

#include "cmd_line_arguments.h"
#include "logging.h"


#ifndef __SPADE_OBJECT_H_INCLUDED__
#define __SPADE_OBJECT_H_INCLUDED__

using namespace std;

struct job_data_type {
    string analysisCode;
    string creationTime;
    string machineName;
    string modificationTime;
    string name;
    string precision;
    vector<string> productAddOns;
    string version;
};

struct sector_definition_type {
    int numSectors;
    string start_point;
    string end_point;
};

struct section_point_type {
    string number;
    string description;
};

struct section_category_type {
    string name;
    string description;
    vector<string> section_point_numbers;
    vector<string> section_point_descriptions;
};

struct user_xy_data_type {
    string name;
    string sourceDescription;
    string contentDescription;
    string positionDescription;
    string xAxisLabel;
    string yAxisLabel;
    string legendLabel;
    string description;
    vector<float> data;
    int row_size;
};

struct tangential_behavior_type {
    string formulation;
    string directionality;
    string slipRateDependency;  // Boolean
    string pressureDependency;  // Boolean
    string temperatureDependency;  // Boolean
    int dependencies;
    string exponentialDecayDefinition;
    vector<vector<double>> table;  // Must remain as 2D vector due to varying sizes of columns
    int max_column_size;
    double shearStressLimit;  // String NONE or a double
    string maximumElasticSlip;
    double fraction;
    double absoluteDistance;  // String NONE or a double
    double elasticSlipStiffness;
    int nStateDependentVars;
    string useProperties;
};

struct element_type {
    vector<int> connectivity;
    section_category_type sectionCategory;
    vector<string> instanceNames;
    set<string> sets;
};

struct elements_type {
    map<string, map<int, element_type>> elements; // accessed like elements_type[type][label] (e.g. elements_type['CAX4T'][1])
    map <string, set<int>> element_sets;
};

struct node_type {
    vector<float> coordinates;
    set<string> sets;
};

struct nodes_type {
    map<int, node_type> nodes;
    map <string, set<int>> node_sets;
};

struct set_type {
    string name;
    string type;  // Enum [NODE_SET, ELEMENT_SET, SURFACE_SET]
    int size;
    vector<string> instanceNames;
    nodes_type* nodes;
    elements_type* elements;
    vector<string> faces;
};

struct contact_standard_type {
    string sliding;  // Symbolic Constant [FINITE, SMALL]
    double smooth;
    double hcrit;
    string limitSlideDistance;
    double slideDistance;
    double extensionZone;
    string adjustMethod;  // Symbolic Constant [NONE, OVERCLOSED, TOLERANCE, SET]
    double adjustTolerance;
    string enforcement;  // Symbolic Constant [NODE_TO_SURFACE, SURFACE_TO_SURFACE]
    string thickness;  // Boolean
    string tied;  // Boolean
    string contactTracking;  // Symbolic Constant [ONE_CONFIG, TWO_CONFIG]
    string createStepName;

    tangential_behavior_type interactionProperty;
    set_type main;
    set_type secondary;
    set_type adjust;
};

struct contact_explicit_type {
    string sliding;  // Symbolic Constant [FINITE, SMALL]
    string mainNoThick;
    string secondaryNoThick;
    string mechanicalConstraint;
    string weightingFactorType;
    double weightingFactor;
    string createStepName;
    string useReverseDatumAxis;  // Boolean
    string contactControls;
  
    tangential_behavior_type interactionProperty;
    set_type main;
    set_type secondary;
 
};

struct tie_type {
    set_type main;
    set_type secondary;
    string adjust;
    string positionToleranceMethod;
    string positionTolerance;
    string tieRotations;
    string constraintRatioMethod;
    string constraintRatio;
    string constraintEnforcement;
    string thickness;
};

struct display_body_type {
    string instanceName;
    string referenceNode1InstanceName;
    string referenceNode1Label;
    string referenceNode2InstanceName;
    string referenceNode2Label;
    string referenceNode3InstanceName;
    string referenceNode3Label;
};

struct datum_csys_type {
    string name;
    string type;
    float x_axis[3];
    float y_axis[3];
    float z_axis[3];
    float origin[3];
};

struct coupling_type {
    set_type surface;
    set_type refPoint;
    set_type nodes;
    string couplingType;
    string weightingMethod;
    string influenceRadius;

    string u1;
    string u2;
    string u3;
    string ur1;
    string ur2;
    string ur3;
};

struct mpc_type {
    set_type surface;
    set_type refPoint;
    string mpcType;
    string userMode;
    string userType;
};

struct shell_solid_coupling_type {
    set_type shellEdge;
    set_type solidFace;
    string positionToleranceMethod;
    string positionTolerance;
    string influenceDistanceMethod;
    string influenceDistance;
};

struct constraint_type {
    vector<tie_type> ties;
    vector<display_body_type> display_bodies;
    vector<coupling_type> couplings;
    vector<mpc_type> mpc;
    vector<shell_solid_coupling_type> shell_solid_couplings;
};

struct part_type {
    string name;
    string embeddedSpace;
    nodes_type* nodes;
    elements_type* elements;
    vector<set_type> nodeSets;
    vector<set_type> elementSets;
    vector<set_type> surfaces;
};

struct section_assignment_type {
  set_type region;
  string sectionName;
};

struct beam_orientation_type {
  string method;
  set_type region;
  vector<float> beam_vector;  // vector is a reserved word, so had to name it something else
};

struct rebar_orientation_type {
  string axis;
  float angle;
  set_type region;
  datum_csys_type csys;
};

struct analytic_surface_segment_type {
    string type;
    vector<float> data;
    int column_size;
    int row_size;
};

struct analytic_surface_type {
    string name;
    string type;
    double filletRadius;
    vector<analytic_surface_segment_type> segments;
    vector<vector<float>> localCoordData;  // Must remain as 2D vector due to varying sizes of columns
    int max_column_size;
};

struct rigid_body_type {
    string position;
    string isothermal;  // Boolean
    set_type referenceNode;
    set_type elements;
    set_type tieNodes;
    set_type pinNodes;
    analytic_surface_type analyticSurface;
};

struct instance_type {
    string name;
    string embeddedSpace;
    nodes_type* nodes;
    elements_type* elements;
    vector<set_type> nodeSets;
    vector<set_type> elementSets;
    vector<set_type> surfaces;
    vector<rigid_body_type> rigidBodies;
    vector<section_assignment_type> sectionAssignments;
    vector<beam_orientation_type> beamOrientations;
    vector<rebar_orientation_type> rebarOrientations;
    analytic_surface_type analyticSurface;
};

struct mesh_type {
    nodes_type nodes;
    elements_type elements;
    int part_index;
    int instance_index;
};

struct connector_orientation_type {
    set_type region;
    string axis1;  // SymbolicConstant
    string axis2;  // SymbolicConstant
    datum_csys_type localCsys1;
    datum_csys_type localCsys2;
    string orient2sameAs1; // Boolean
    float angle1;
    float angle2;
};

struct assembly_type {
    string name;
    string embeddedSpace;
    nodes_type* nodes;
    elements_type* elements;
    vector<set_type> nodeSets;
    vector<set_type> elementSets;
    vector<set_type> surfaces;
    vector<instance_type> instances;
    vector<datum_csys_type> datumCsyses;
    vector<connector_orientation_type> connectorOrientations;
};

struct field_location_type {
    string position;
    vector<section_point_type> sectionPoint;
};

struct field_value_type {
    vector<int> elementLabel;
    vector<int> nodeLabel;
    vector<int> integrationPoint;
    vector<string> type;
    vector<float> magnitude;
    vector<float> tresca;
    vector<float> press;
    vector<float> inv3;
    vector<float> maxPrincipal;
    vector<float> midPrincipal;
    vector<float> minPrincipal;
    vector<float> maxInPlanePrincipal;
    vector<float> minInPlanePrincipal;
    vector<float> outOfPlanePrincipal;
    vector<string> sectionPointNumber;
    vector<string> sectionPointDescription;
    bool magnitudeEmpty;
    bool trescaEmpty;
    bool pressEmpty;
    bool inv3Empty;
    bool maxPrincipalEmpty;
    bool midPrincipalEmpty;
    bool minPrincipalEmpty;
    bool maxInPlanePrincipalEmpty;
    bool minInPlanePrincipalEmpty;
    bool outOfPlanePrincipalEmpty;

    bool elementEmpty;
    bool nodeEmpty;
    bool integrationPointEmpty;
    bool typeEmpty;
    bool sectionPointNumberEmpty;
    bool sectionPointDescriptionEmpty;
};

struct field_bulk_type {
    string position;
    string precision;
    int orientationWidth;
    int numberOfElements;
    int length;
    int valuesPerElement;
    int width;
    string baseElementType;
    vector<vector<string>> faces;
    vector<int> elementLabels;
    vector<int> nodeLabels;
    vector<int> integrationPoints;
    vector<float> data;
    vector<double> dataDouble;
    vector<float> conjugateData;
    vector<double> conjugateDataDouble;
    vector<float> localCoordSystem;
    vector<double> localCoordSystemDouble;
    vector<float> mises;
    vector<string> componentLabels;
    bool emptyFaces;  // Keep track of whether any face data exists
};

struct field_data_type {
    field_value_type fieldValues;
    vector<field_bulk_type> bulkValues;
};

struct field_output_type {
    string name;
    string description;
    string type;
    vector<string> componentLabels;
    vector<string> validInvariants;
    int dim;
    int dim2;
//    string isEngineeringTensor;  // Boolean
    vector<field_location_type> locations;
    map<string, field_data_type> values;  // String index is the name of the instance
    int max_width;
    int max_length;
    bool isComplex;  // Flag indicating if there is conjugate data
};

struct frame_type {
    int incrementNumber;
    int cyclicModeNumber;
    int mode;
    string description;
    string domain;
    float frameValue;
    float frequency;
    string loadCase;
    vector<field_output_type> fieldOutputs;
    int max_width;
    int max_length;
    bool skip;  // Don't process if a frame-value is specified on the command line that approximately matches the frameValue or frequency
};

struct history_point_type {
    element_type element;
    int element_label;
    string elementType;
    bool hasElement;
    bool hasNode;
    int ipNumber;
    section_point_type sectionPoint;
    string face;
    string position;
    int node_label;
    float node_coordinates[3];
    set_type region;
    string assemblyName;  // Just storing the name not the entire assembly
    string instanceName;  // Just storing the name not the entire instance
};

struct history_output_type {
    string description;
    string type;
    string name;
    vector<float> data;
    int row_size;
    vector<float> conjugateData;
    int row_size_conjugate;
};

struct history_region_type {
    string name;
    string description;
    string position;
    history_point_type point;
    string loadCase;
    vector<history_output_type> historyOutputs;
};

struct step_type {
    string name;
    string description;
    string domain;
    string previousStepName;
    string procedure;
    string nlgeom; // Boolean
    int number;
    double timePeriod;
    double totalTime;
    double mass;
    double acousticMass;
    vector<frame_type> frames;
    vector<history_region_type> historyRegions;
    vector<string> loadCases;
    vector<double> massCenter;
    vector<double> acousticMassCenter;
    double inertiaAboutCenter[6];
    double inertiaAboutOrigin[6];
};

/*!
   This class holds all the data from the odb
*/
class SpadeObject {
    public:
        //! The constructor.
        /*!
          The constructor checks to see if the odb file needs to be upgraded, upgrades if necessary, then opens it and calls the function to process the odb
          \param command_line_arguments CmdLineArguments object storing command line arguments
          \param log_file Logging object for writing log messages
          \sa process_odb_without_steps()
        */
        SpadeObject (CmdLineArguments &command_line_arguments, Logging &log_file);
        //! Process all but the steps data from the open odb file and store the results
        /*!
          After the odb has been opened this function will do the parsing, including calling of other functions needed for parsing
          \param odb An open odb object
        */
        void process_odb_without_steps (odb_Odb &odb);
        //! Process odb_SectionCategory object from the odb file
        /*!
          Process odb_SectionCategory object and return the values in a section_category_type
          \param section_category An odb_SectionCategory object in the odb
          \return section_category_type with data stored from the odb
          \sa process_odb()
        */
        section_category_type process_section_category (const odb_SectionCategory &section_category);
        //! Process interaction property object from the odb file
        /*!
          Process interaction property object and return the values in a tangential_behavior_type
          \param interaction_property An odb_InteractionProperty object in the odb
          \return tangential_behavior_type with data stored from the odb
          \sa process_interactions()
          \sa process_constraints()
          \sa process_odb()
        */
        tangential_behavior_type process_interaction_property (const odb_InteractionProperty &interaction_property);
        //! Process odb_Node objects from the odb file
        /*!
          Process odb_Node objects, save node data in multiple vectors inside a map, return pointer to location in map
          \param nodes An odb_SequenceNode object in the odb
          \param instance_name An instance name where this node might be found
          \param assembly_name An assembly name where this node might be found
          \param set_name A set name where this node might be found
          \param part_name A part name where this node might be found
          \param embedded_space An int indicating what type of dimenions are being used
          \return nodes_type pointer to map that stores node data
          \sa process_odb()
        */
        nodes_type* process_nodes (const odb_SequenceNode &nodes, const string &instance_name, const string &assembly_name, const string &set_name, const string &part_name, const int &embedded_space);
        //! Process odb_Element objects from the odb file
        /*!
          Process odb_Element objects, save element data in data map, return pointer to location in map
          \param elements An odb_Element object in the odb
          \param instance_name An instance name where this node might be found
          \param assembly_name An assembly name where this node might be found
          \param set_name A set name where this node might be found
          \param part_name A part name where this node might be found
          \return elements_type pointer to map that stores element data
          \sa process_odb()
        */
        elements_type* process_elements (const odb_SequenceElement &elements, const string &instance_name, const string &assembly_name, const string &set_name, const string &part_name);
        //! Process odb set object from the odb file
        /*!
          Process odb set object and return the values in an set_type
          \param odb_set An odb_Set object in the odb
          \return set_type with data stored from the odb
          \sa process_odb()
        */
        set_type process_set (const odb_Set &odb_set);
        //! Process interactions from the odb file
        /*!
          Process interactions and store the results
          \param interactions A repository of interactions in the odb
          \param odb An open odb object
          \sa process_odb()
        */
        void process_interactions (const odb_InteractionRepository &interactions, odb_Odb &odb);
        //! Process constraints from the odb file
        /*!
          Process constraints and store the results
          \param constraints A repository of constraints in the odb
          \param odb An open odb object
          \sa process_odb()
        */
        void process_constraints (const odb_ConstraintRepository &constraints, odb_Odb &odb);
        //! Process a part from the odb file
        /*!
          Process a part object and store the results
          \param part An odb part object
          \param odb An open odb object
          \return part_type with data stored from the odb
          \sa process_odb()
        */
        part_type process_part (const odb_Part &part, odb_Odb &odb);
        //! Process a section assignment from the odb file
        /*!
          Process a section assignment and store the results
          \param section_assingment An odb section assignment object
          \return section_assignment_type with data stored from the odb
          \sa process_odb()
        */
        section_assignment_type process_section_assignment (const odb_SectionAssignment &section_assignment);
        //! Process a beam orientation from the odb file
        /*!
          Process a beam orientation and store the results
          \param beam_orientation An odb beam orientation object
          \return beam_orientation_type with data stored from the odb
          \sa process_odb()
        */
        beam_orientation_type process_beam_orientation (const odb_BeamOrientation &beam_orientation);
        //! Process a rebar orientation from the odb file
        /*!
          Process a rebar orientation and store the results
          \param rebar_orientation An odb rebar orientation object
          \return rebar_orientation_type with data stored from the odb
          \sa process_odb()
        */
        rebar_orientation_type process_rebar_orientation (const odb_RebarOrientation &rebar_orientation);
        //! Process a datum csys from the odb file
        /*!
          Process a datum csys and store the results
          \param datum_csys An odb datum csys object
          \return datum_csys_type with data stored from the odb
          \sa process_odb()
        */
        datum_csys_type process_csys (const odb_DatumCsys &datum_csys);
        //! Process an analytic surface segment from the odb file
        /*!
          Process an analytic surface segment and store the results
          \param segment An odb analytic surface segment object
          \return analytic_surface_segment_type with data stored from the odb
          \sa process_odb()
        */
        analytic_surface_segment_type process_segment (const odb_AnalyticSurfaceSegment &segment);
        //! Process an analytic surface from the odb file
        /*!
          Process an analytic surface and store the results
          \param analytic_surface An odb analytic surface object
          \return analytic_surface_type with data stored from the odb
          \sa process_odb()
        */
        analytic_surface_type process_analytic_surface (const odb_AnalyticSurface &analytic_surface);
        //! Process a rigid body from the odb file
        /*!
          Process a rigid body and store the results
          \param rigid_body An odb rigid body object
          \return rigid_body_type with data stored from the odb
          \sa process_odb()
        */
        rigid_body_type process_rigid_body (const odb_RigidBody &rigid_body);
        //! Process an instance from the odb file
        /*!
          Process an instance and store the results
          \param instance An odb instance object
          \param odb An open odb object
          \return instance_type with data stored from the odb
          \sa process_odb()
        */
        instance_type process_instance (const odb_Instance &instance, odb_Odb &odb);
        //! Process a connector orientation object from the odb file
        /*!
          Process a connector orientation and store the results
          \param connector_orientation An odb connector orientation object
          \return connector_orientation_type with data stored from the odb
          \sa process_odb()
        */
        connector_orientation_type process_connector_orientation (const odb_ConnectorOrientation &connector_orientation);
        //! Process an assembly from the odb file
        /*!
          Process  an assembly and store the results
          \param assembly An odb assembly object
          \param odb An open odb object
          \return assembly_type with data stored from the odb
          \sa process_odb()
        */
        assembly_type process_assembly (odb_Assembly &assembly, odb_Odb &odb);
        //! Process field value from the odb file
        /*!
          Process a field value object and store the results
          \param field_value An odb field value object
          \param invariants An odb sequence invariant object
          \param values Data structure for storing processed data
          \return field_value_type with data stored from the odb
          \sa process_odb()
        */
        void process_field_values(const odb_FieldValue &field_value, const odb_SequenceInvariant& invariants, field_value_type &values);
        //! Process field bulk data from the odb file
        /*!
          Process a field bulk data object and store the results
          \param field_bulk_data An odb field bulk data object
          \param invariants An odb sequence invariant object
          \param complex_data Boolean indicating if the data is complex
          \return field_bulk_type with data stored from the odb
          \sa process_odb()
        */
        field_bulk_type process_field_bulk_data(const odb_FieldBulkData &field_bulk_data, const odb_SequenceInvariant& invariants, bool complex_data);
        //! Process field output from the odb file
        /*!
          Process a field output object and store the results
          \param field_output An odb field output object
          \return field_output_type with data stored from the odb
          \sa process_odb()
        */
        field_output_type process_field_output (const odb_FieldOutput &field_output);
        //! Process a frame from the odb file
        /*!
          Process a frame object and store the results
          \param frame An odb frame object
          \return frame_type with data stored from the odb
          \sa process_odb()
        */
        frame_type process_frame (const odb_Frame &frame);
        //! Process a history point from the odb file
        /*!
          Process a history region point and store the results
          \param history_point An odb history point object
          \return history_point_type with data stored from the odb
          \sa process_odb()
        */
        history_point_type process_history_point (const odb_HistoryPoint history_point);
        //! Process a history output from the odb file
        /*!
          Process a history output object and store the results
          \param history_output An odb history output object
          \return history_output_type with data stored from the odb
          \sa process_odb()
        */
        history_output_type process_history_output (const odb_HistoryOutput &history_region);
        //! Process a history region from the odb file
        /*!
          Process a history region object and store the results
          \param history_region An odb history region object
          \return history_region_type with data stored from the odb
          \sa process_odb()
        */
        history_region_type process_history_region (const odb_HistoryRegion &history_region);
        //! Process a step from the odb file
        /*!
          Process a step object and store the results
          \param step An odb step object
          \param odb An open odb object
          \return step_type with data stored from the odb
          \sa process_odb()
        */
        step_type process_step (const odb_Step &step, odb_Odb &odb);
        //! Process a tie constraint from the odb file
        /*!
          Process a Tie object and store the results
          \param tie An odb Tie object
          \sa process_odb()
        */
        tie_type process_tie (const odb_Tie &tie);
        //! Process a display body constraint from the odb file
        /*!
          Process a display body object and store the results
          \param display_body An odb display body object
          \sa process_odb()
        */
        display_body_type process_display_body (const odb_DisplayBody &display_body);
        //! Process a coupling constraint from the odb file
        /*!
          Process a coupling object and store the results
          \param coupling An odb coupling object
          \sa process_odb()
        */
        coupling_type process_coupling (const odb_Coupling &coupling);
        //! Process a mpc from the odb file
        /*!
          Process a mpc object and store the results
          \param mpc An odb mpc object
          \sa process_odb()
        */
        mpc_type process_mpc (const odb_MPC &mpc);
        //! Process a shell solid coupling constraint from the odb file
        /*!
          Process a shell solid coupling object and store the results
          \param shell_solid_coupling An odb shell solid coupling object
          \sa process_odb()
        */
        shell_solid_coupling_type process_shell_solid_coupling (const odb_ShellSolidCoupling &shell_solid_coupling);
        //! Process and write the step data from the odb
        /*!
          With the odb file still open and the h5 file open, loop through the history and field output data and read then write,
          rather than store all the data in memory then write it later
          \param odb An open odb object
          \param h5_file Open h5_file object for writing
        */
        void process_and_write_step_data_h5 (odb_Odb &odb, H5::H5File &h5_file);
        //! Process and write the history output data from the odb
        /*!
          With the odb and the h5 file open, loop through and read then write the history output data
          \param odb An open odb object
          \param step An odb step object
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void process_and_write_history_data_h5 (odb_Odb &odb, H5::H5File &h5_file, const odb_Step &step, const string &group_name);


        //Functions for writing out the data

        //! Write non-step output to a YAML file.
        /*!
          Write the parsed odb data to a YAML formatted file
          \sa SpadeObject()
        */
        void write_yaml_without_steps ();
        //! Write non-step output to a JSON file.
        /*!
          Write the parsed odb data to a JSON formatted file
          \sa SpadeObject()
        */
        void write_json_without_steps ();
        //! Write non-step output to an HDF5 file.
        /*!
          Write the parsed odb data to an HDF5 formatted file
          \param h5_file Open h5_file object for writing
          \sa SpadeObject()
        */
        void write_h5_without_steps (H5::H5File &h5_file);
        //! Open hdf5 subgroup and create it and it's parent groups if they don't exist
        /*!
          Open and return hdf5 group, if it doesn't exist then create it, if it's parent groups don't exist create them
          \param h5_file Open h5_file object for writing
          \param sub_group_name Name of the sub group to open or create
          \param exists Boolean indicating if the sub group already exists
        */
        H5::Group open_subgroup(H5::H5File &h5_file, const string &sub_group_name, bool &exists);
        //! Write mesh data to an HDF5 file
        /*!
          Write mesh data in an extract format where all element and node data is in a single place
          \param h5_file Open h5_file object for writing
        */
        void write_mesh(H5::H5File &h5_file);
        //! Write mesh node data to an HDF5 file
        /*!
          Write mesh node data in an extract format
          \param h5_file Open h5_file object for writing
          \param group HDF5 group in which to write the new data
          \param nodes node data to be written
          \param embedded_space string indicating what type of embedded space, which indicates the dimensions
        */
        void write_mesh_nodes(H5::H5File &h5_file, H5::Group &group, map<int, node_type> nodes, const string embedded_space);
        //! Write mesh element data to an HDF5 file
        /*!
          Write mesh element data in an extract format
          \param h5_file Open h5_file object for writing
          \param group HDF5 group in which to write the new data
          \param elements element data to be written
        */
        void write_mesh_elements(H5::H5File &h5_file, H5::Group &group, map<string, map<int, element_type>> elements);
        //! Write history region in extract format to HDF5 file
        /*!
          Write history region in extract format for given step
          \param h5_file Open h5_file object for writing
          \param history_region Data to be written
          \param step_group_name Name of current step
        */
        void write_extract_history_region(H5::H5File &h5_file, history_region_type &history_region, string &step_group_name);
        //! Write field output in extract format to HDF5 file
        /*!
          Write field output in extract format for given step
          \param h5_file Open h5_file object for writing
          \param step Current Step for which to write field output
        */
        void write_extract_field_output(H5::H5File &h5_file, step_type &step);
        //! Write parts data to an HDF5 file
        /*!
          Write parts data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void write_parts(H5::H5File &h5_file, const string &group_name);
        //! Write assembly data to an HDF5 file
        /*!
          Write assembly data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void write_assembly(H5::H5File &h5_file, const string &group_name);
        //! Write field bulk data to an HDF5 file
        /*!
          Write field bulk data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param field_bulk_data Data to be written
          \param complex_data Boolean indicating if the data is complex
        */
        void write_field_bulk_data(H5::H5File &h5_file, const string &group_name, field_bulk_type &field_bulk_data, bool complex_data);
        //! Write field value data to an HDF5 file
        /*!
          Write field value data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param group Group where data is to be written
          \param values Data to be written
        */
        void write_field_values(H5::H5File &h5_file, const string &group_name, H5::Group &group, field_value_type &values);
        //! Write field output data to an HDF5 file
        /*!
          Write field output data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param field_output Data to be written
        */
        void write_field_output(H5::H5File &h5_file, const string &group_name, field_output_type &field_output);
        //! Write frames data to an HDF5 file
        /*!
          Write frames data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param frame_group Group where data is to be written
          \param frames Data to be written
        */
        void write_frame(H5::H5File &h5_file, H5::Group &frame_group, frame_type &frame);
        //! Write history point data to an HDF5 file
        /*!
          Write history point data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param history_point Data to be written
        */
        void write_history_point(H5::H5File &h5_file, const string &group_name, history_point_type &history_point);
        //! Write history output data to an HDF5 file
        /*!
          Write history output data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param history_output Data to be written
        */
        void write_history_output(H5::H5File &h5_file, const string &group_name, history_output_type &history_output);
        //! Write history region data to an HDF5 file
        /*!
          Write history region data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param history_region Data to be written
        */
        void write_history_region(H5::H5File &h5_file, const string &group_name, history_region_type &history_region);
        //! Write steps data to an HDF5 file
        /*!
          Write steps data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param step_group Group where data is to be written
          \param step Step object to write
        */
        void write_step(H5::H5File &h5_file, H5::Group &step_group, step_type& step);
        //! Write instances data to an HDF5 file
        /*!
          Write instances data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void write_instances(H5::H5File &h5_file, const string &group_name);
        //! Write instance data to an HDF5 file
        /*!
          Write instance data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group Group where data is to be written
          \param group_name Name of the group where data is to be written
          \param instance Data to be written
        */
        void write_instance(H5::H5File &h5_file, H5::Group &group, const string &group_name, instance_type instance);
        //! Write Analytic Surface data to an HDF5 file
        /*!
          Write Analytic Surface data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param analytic_surface Data to be written
        */
        void write_analytic_surface(H5::H5File &h5_file, const string &group_name, analytic_surface_type &analytic_surface);
        //! Write Datum Csys data to an HDF5 file
        /*!
          Write Datum Csys data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param datum_csys Data to be written
        */
        void write_datum_csys(H5::H5File &h5_file, const string &group_name, const datum_csys_type &datum_csys);
        //! Write constraints data to an HDF5 file
        /*!
          Write different types of constraint data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void write_constraints(H5::H5File &h5_file, const string &group_name);
        //! Write tangential behavior data to an HDF5 file
        /*!
          Write tangential behavior data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param tangential_behavior Data to be written
        */
        void write_tangential_behavior(H5::H5File &h5_file, const string &group_name, tangential_behavior_type& tangential_behavior);
        //! Write interactions data to an HDF5 file
        /*!
          Write standard and or explicit interaction data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
        */
        void write_interactions(H5::H5File &h5_file, const string &group_name);
        //! Write elements data to an HDF5 file
        /*!
          Write vector of element data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group HDF5 group in which to write the new data
          \param group_name Name of the group where data is to be written
          \param elements Element data to be written
          \param set_name String with the name of the set if given
        */
        void write_elements(H5::H5File &h5_file, H5::Group &group, const string &group_name, const elements_type* elements, const string &set_name);
        //! Write node data to an HDF5 file
        //! Write nodes data to an HDF5 file
        /*!
          Write vector of node data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group HDF5 group in which to write the new data
          \param group_name Name of the group where data is to be written
          \param nodes Node data to be written
          \param set_name String with the name of the set if given
        */
        void write_nodes(H5::H5File &h5_file, H5::Group &group, const string &group_name, const nodes_type* nodes, const string &set_name);
        //! Write sets data to an HDF5 file
        /*!
          Write vector of set data into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param sets Vector of set data to be written
        */
        void write_sets(H5::H5File &h5_file, const string &group_name, const vector<set_type> &sets);
        //! Write set data to an HDF5 file
        /*!
          Write data from a set type into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \param odb_set Set data to be written
        */
        void write_set(H5::H5File &h5_file, const string &group_name, const set_type &odb_set);
        //! Write a section category type to an HDF5 file
        /*!
          Write data from section category type into an HDF5 file
          \param h5_file Open h5_file object for writing
          \param group Name of HDF5 group in which to write the new data
          \param group_name Name of the group where data is to be written
          \param section_category The section category data to be written
        */
        void write_section_category(H5::H5File &h5_file, const H5::Group &group, const string &group_name, const section_category_type &section_category);
        //! Write a string as an attribute
        /*!
          Create an attribute with a string using the passed-in values
          \param group Name of HDF5 group in which to write the new attribute
          \param attribute_name Name of the new attribute where a string is to be written
          \param string_value The string that should be written in the new attribute
        */
        void write_attribute(const H5::Group &group, const string &attribute_name, const string &string_value);
        //! Write attributes for xarray to recgonize the dataset as part of an xarray
        /*!
          Write the passed-in values as attributes to the
          \param group Name of HDF5 group in which to write the new attribute
          \param dataset_name Name of HDF5 dataset in which to write the new attribute
          \param name value of NAME attribute
          \param class_name value of CLASS attribute
          \param fill_value value of _FillValue attribute
          \param coordinates value of coordinates attribute
          \param description value of description attribute
        */
        void write_xarray_attributes(const H5::Group &group, const string &dataset_name, const string &name, const string &class_name, const string &fill_value, const string &coordinates, const string &description, const string &dim_id);
        //! Write a vector of strings as an attribute
        /*!
          Create an attribute with a vector of strings using the passed-in values
          \param h5_file Open h5_file object for writing
          \param group Name of HDF5 group in which to write the new attribute
          \param attribute_name Name of the new attribute where a string is to be written
          \param string_values The strings that should be written in the new attribute
        */
        void write_vector_attribute(const H5::Group &group, const string &attribute_name, const vector<string> &string_values);
        //! Write a string as a dataset
        /*!
          Create a dataset with a single string using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a string is to be written
          \param string_value The string that should be written in the new dataset
        */
        void write_string_dataset(const H5::Group &group, const string &dataset_name, const string &string_value);
        //! Write a vector of strings as a dataset
        /*!
          Create a dataset with an array of strings using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where an array of strings is to be written
          \param string_values The vector of strings that should be written in the new dataset
        */
        void write_string_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<string> &string_values);
        //! Write an integer as a dataset
        /*!
          Create a dataset with an integer using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where an integer is to be written
          \param int_value The integer that should be written in the new dataset
        */
        //! Write an array of arrays of integers as a dataset
        void write_integer_dataset(const H5::Group &group, const string &dataset_name, const int &int_value);
        /*!
          Create a dataset with a two-dimensional array of strings using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_row_size Integer indicating the row dimension
          \param max_column_size Integer indicating the column dimension
          \param string_array A pointer to the array of arrays of strings that should be written in the new dataset
        */
        void write_string_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, string *string_array);
        //! Write a vector of vectors of strings as a dataset
        /*!
          Create a dataset with a two-dimensional array of strings using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_column_size Integer indicating the column dimension, row dimension is determined by size of data_array
          \param string_data The vector of vectors of strings that should be written in the new dataset
          \sa write_string_2D_array()
        */
        void write_string_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, vector<vector<string>> & string_data);
        //! Write an integer array as a dataset
        /*!
          Create a dataset with an array of integers using the passed-in value
          \param h5_file Open h5_file object for writing
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param int_array The integer array that should be written in the new dataset
        */
        void write_integer_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const int* int_array);
        //! Write an integer vector as a dataset
        /*!
          Create an integer array from an integer vector and call write_integer_array_dataset
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param int_data The integer data that should be written in the new dataset
          \sa write_integer_array_dataset()
        */
        void write_integer_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<int> &int_data);
        //! Write an array of arrays of integers as a dataset
        /*!
          Create a dataset with a two-dimensional array of integers using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_row_size Integer indicating the row dimension
          \param max_column_size Integer indicating the column dimension
          \param integer_array A pointer to the array of arrays of integers that should be written in the new dataset
        */
        void write_integer_2D_array(const H5::Group& group, const string & dataset_name, const int &row_size, const int &column_size, int *integer_array);
        //! Write an arrays of arrays of integers as a dataset
        /*!
          Create a dataset with a two-dimensional array of integers using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param integer_array A vector of integers should be written in the new dataset
        */
        void write_integer_2D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, const vector<int> &integer_data);
        //! Write a vector of vectors of integers as a dataset
        /*!
          Create a dataset with a two-dimensional array of integers using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_column_size Integer indicating the column dimension, row dimension is determined by size of data_array
          \param integer_array The vector of vectors of integers that should be written in the new dataset
          \sa write_integer_2D_array()
        */
        void write_integer_2D_vector(const H5::Group& group, const string & dataset_name, const int & max_column_size, vector<vector<int>> & integer_data);
        //! Write an float as a dataset
        /*!
          Create a dataset with a float using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a float is to be written
          \param float_value The float that should be written in the new dataset
        */
        void write_float_dataset(const H5::Group &group, const string &dataset_name, const float &float_value);
        //! Write a float array as a dataset
        /*!
          Create a dataset with an array of floats using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param float_array The float array that should be written in the new dataset
        */
        void write_float_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const float* float_array);
        //! Write an float vector as a dataset
        /*!
          Create a float array from a float vector and call write_float_array_dataset
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param float_data The float data that should be written in the new dataset
          \sa write_float_array_dataset()
        */
        void write_float_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<float> &float_data);
        //! Write an array of arrays of floats as a dataset
        /*!
          Create a dataset with a two-dimensional array of floats using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param float_array A pointer to the array of arrays of floats that should be written in the new dataset
        */
        void write_float_2D_array(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, float *float_array);
        //! Write an arrays of arrays of floats as a dataset
        /*!
          Create a dataset with a two-dimensional array of floats using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param float_array A vector of floats should be written in the new dataset
        */
        void write_float_2D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, const vector<float> &float_data);
        //! Write an array of arrays of arrays of floats as a dataset
        /*!
          Create a dataset with a three-dimensional array of floats using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a three-dimensional array is to be written
          \param aisle_size Integer indicating the aisle dimension
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param float_array A vector of floats should be written in the new dataset
        */
        void write_float_3D_data(const H5::Group &group, const string &dataset_name, const int &aisle_size, const int &row_size, const int &column_size, const vector<float> &float_data);
        //! Write a vector of vectors of floats as a dataset
        /*!
          Create a dataset with a two-dimensional array of floats using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_column_size Integer indicating the column dimension, row dimension is determined by size of data_array
          \param data_array The vector of vectors of floats that should be written in the new dataset
          \sa write_float_2D_array()
        */
        void write_float_2D_vector(const H5::Group &group, const string &dataset_name, const int &max_column_size, vector<vector<float>> &data_array);
        //! Write an double as a dataset
        /*!
          Create a dataset with a double using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a double is to be written
          \param double_value The double that should be written in the new dataset
        */
        void write_double_dataset(const H5::Group &group, const string &dataset_name, const double &double_value);
        //! Write a double array as a dataset
        /*!
          Create a dataset with an array of doubles using the passed-in value
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param double_array The double array that should be written in the new dataset
        */
        void write_double_array_dataset(const H5::Group &group, const string &dataset_name, const int array_size, const double* double_array);
        //! Write a double vector as a dataset
        /*!
          Create a double array from a double vector and call write_double_array_dataset
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where the data is to be written
          \param double_data The double data that should be written in the new dataset
          \sa write_double_array_dataset()
        */
        void write_double_vector_dataset(const H5::Group &group, const string &dataset_name, const vector<double> &double_data);
        //! Write an array of arrays of doubles as a dataset
        /*!
          Create a dataset with a two-dimensional array of doubles using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param double_array A pointer to the array of arrays of doubles that should be written in the new dataset
        */
        void write_double_2D_array(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, double *double_array);
        //! Write an array of arrays of doubles as a dataset
        /*!
          Create a dataset with a two-dimensional array of doubles using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param double_array A vector of doubles that should be written in the new dataset
        */
        void write_double_2D_data(const H5::Group &group, const string &dataset_name, const int &row_size, const int &column_size, const vector<double> &double_data);
        //! Write an array of arrays of arrays of doubles as a dataset
        /*!
          Create a dataset with a three-dimensional array of doubles using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a three-dimensional array is to be written
          \param aisle_size Integer indicating the aisle dimension
          \param row_size Integer indicating the row dimension
          \param column_size Integer indicating the column dimension
          \param double_array A vector of doubles that should be written in the new dataset
        */
        void write_double_3D_data(const H5::Group &group, const string &dataset_name, const int &aisle_size, const int &row_size, const int &column_size, const vector<double> &double_data);
        //! Write a vector of vectors of doubles as a dataset
        /*!
          Create a dataset with a two-dimensional array of doubles using the passed-in values
          \param group Name of HDF5 group in which to write the new dataset
          \param dataset_name Name of the new dataset where a two-dimensional array is to be written
          \param max_column_size Integer indicating the column dimension, row dimension is determined by size of data_array
          \param data_array The vector of vectors of doubles that should be written in the new dataset
          \sa write_double_2D_array()
        */
        void write_double_2D_vector(const H5::Group &group, const string &dataset_name, const int &max_column_size, vector<vector<double>> &data_array);
        //! Create a group in the hdf5 file, log warning if not possible
        /*!
          \param h5_file Open h5_file object for writing
          \param group_name Name of the group where data is to be written
          \return group that's been created
        */
        H5::Group create_group(H5::H5File &h5_file, const string &group_name);
        //! Replace forward slashes '/' with vertical bars '|' from strings that will be used in group name paths
        /*!
          \param name String with potential slashes to be replaced
          \return string that's had forward slashes replaced
        */
        string replace_slashes(const string &name);


    private:
        string name;
        string analysisTitle;
        string description;
        string path;
        string isReadOnly;
        job_data_type job_data;
        vector<part_type> parts;
        sector_definition_type sector_definition;
        vector<section_category_type> section_categories;
        vector<user_xy_data_type> user_xy_data;
        vector<contact_standard_type> standard_interactions;
        vector<contact_explicit_type> explicit_interactions;
        constraint_type constraints;
        assembly_type root_assembly;
        map<string, mesh_type> instance_mesh;
        map<string, mesh_type> part_mesh;
        map<string, element_type> elements;
        map<string, string> node_links;
        map<string, string> element_links;
        map<string, string> instance_links;

        string dimension_enum_strings[4];
        string faces_enum_strings[35];
        string default_instance_name;
        CmdLineArguments* command_line_arguments;
        Logging* log_file;
};
#endif  // __SPADE_OBJECT_H_INCLUDED__
