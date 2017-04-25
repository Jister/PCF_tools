#ifndef UFD_ROUTE_PCF_GENERATION_H_INCLUDED
#define UFD_ROUTE_PCF_GENERATION_H_INCLUDED

#include "UFD_Routing_Plugins_Exports.h"
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uf.h>
#include <uf_assem.h>
#include <uf_attr.h>
#include <uf_cfi.h>
#include <uf_cfi_types.h>
#include <uf_csys.h>
#include <uf_defs.h>
#include <uf_error_bases.h>
#include <uf_modl.h>
#include <uf_obj.h>
#include <uf_object_types.h>
#include <uf_part.h>
#include <uf_route.h>
#include <uf_udobj.h>
#include <uf_ui.h>
#include <uf_ui_types.h>
#include <uf_vec.h>
#include <NXOpen/Annotations_AnnotationManager.hxx>
#include <NXOpen/Annotations_Associativity.hxx>
#include <NXOpen/Annotations_CoordinateNote.hxx>
#include <NXOpen/Annotations_DraftingNoteBuilder.hxx>
#include <NXOpen/Annotations_Label.hxx>
#include <NXOpen/Annotations_LabelCollection.hxx>
#include <NXOpen/Annotations_LeaderBuilder.hxx>
#include <NXOpen/Annotations_LeaderDataList.hxx>
#include <NXOpen/Annotations_LeaderData.hxx>
#include <NXOpen/Annotations_Note.hxx>
#include <NXOpen/Annotations_NoteCollection.hxx>
#include <NXOpen/Annotations_PmiAttributeCollection.hxx>
#include <NXOpen/Annotations_PmiAttribute.hxx>
#include <NXOpen/Annotations_PmiManager.hxx>
#include <NXOpen/Assemblies_Component.hxx>
#include <NXOpen/Assemblies_ComponentAssembly.hxx>
#include <NXOpen/Assemblies_ComponentGroup.hxx>
#include <NXOpen/Assemblies_ComponentGroupCollection.hxx>
#include <NXOpen/AttributeManager.hxx>
#include <NXOpen/AttributePropertiesBuilder.hxx>
#include <NXOpen/ErrorList.hxx>
#include <NXOpen/Features_DatumCsys.hxx>
#include <NXOpen/Features_DatumFeature.hxx>
#include <NXOpen/Features_Feature.hxx>
#include <NXOpen/Features_FeatureCollection.hxx>
#include <NXOpen/ModelingView.hxx>
#include <NXOpen/ModelingViewCollection.hxx>
#include <NXOpen/SelectDisplayableObject.hxx>
#include <NXOpen/Session.hxx>
#include <NXOpen/Part.hxx>
#include <NXOpen/PartCollection.hxx>
#include <NXOpen/Point.hxx>
#include <NXOpen/Routing_FittingPort.hxx>
#include <NXOpen/Routing_FittingPortCollection.hxx>
#include <NXOpen/Routing_Port.hxx>
#include <NXOpen/Routing_PortCollection.hxx>
#include <NXOpen/Routing_RouteManager.hxx>
#include <NXOpen/Routing_Stock.hxx>
#include <NXOpen/Routing_StockCollection.hxx>


/*
** PCF string formats
*/
#define HEADER_FMT         "ISOGEN-FILES %s\nUNITS-BORE %s\nUNITS-CO-ORDS %s\nUNITS-BOLT-DIA %s\nUNITS-BOLT-LENGTH %s\nUNITS-WEIGHT %s\n"
#define PIPELINE_REF_FMT   "PIPELINE-REFERENCE %s\n"
#define DATE_FMT           "    DATE-DMY %s\n"
#define COMP_END_POINT_FMT      "    END-POINT %.4f %.4f %.4f %.4f\n"
#define COMP_ITEM_CODE_FMT      "    ITEM-CODE %s\n"
#define COMP_SKEY_FMT           "    SKEY %s\n"
#define COMP_WEIGHT_FMT         "    WEIGHT %.4f\n"
#define COMP_CENTER_POINT_FMT   "    CENTRE-POINT %.4f %.4f %.4f\n"
#define COMP_ANGLE_FMT          "    ANGLE %.2f\n"
#define COMP_BRANCH_POINT_FMT   "    BRANCH1-POINT %.4f %.4f %.4f %.4f\n"
#define COMP_SPINDLE_DIRECT_FMT "    SPINDLE-DIRECTION %s\n"
#define COMP_BRANCH1_POINT_FMT  "    BRANCH1-POINT %.4f %.4f %.4f %.4f\n" 
#define COMP_BRANCH2_POINT_FMT  "    BRANCH2-POINT %.4f %.4f %.4f %.4f\n" 
#define MAT_ITEM_CODE_FMT  "ITEM-CODE %s\n"
#define MAT_DESC_FMT       "    DESCRIPTION %s\n"
#define MAT_MATERIAL_FMT   "    MATERIAL %s\n"
#define MAT_SCHEDULE_FMT   "    SCHEDULE %s\n"
#define MAT_CLASS_FMT      "    CLASS %s\n"
#define MAT_RATING_FMT     "    RATING %s\n"

/*
** Misc. values
*/
#define PISOGEN_ENV_VAR          "PISOGEN"
#define DEFAULT_ISOGEN_FILES     "isogen.fls"
#define DEFAULT_OPTIONS_FILE     "final.opl"
#define DEFAULT_MISC_COMP_SKEY   "FTPL"                  /* fixed pipe; plain */
#define BEND_PIPE_ID             "BEND" 
#define BEND_PIPE_SKEY           "PB"                  /* bend, plain end */
#define ISO_BLOCK_SIZE           10
#define PCF_EXTENSION            ".pcf"
#define TEMP_MAT_EXTENSION       ".mat"
#define TEMFILES_DIR             "temfiles"
#define PISOGEN_DIR              "pisogen"
#define PROJECTS_DIR             "projects"
#define DEFAULT_ISO_TYPE         "final"
#define DEFAULT_PROJECT          "default"
#define FLS_OPTIONS_KEYWORD      "OPTION-SWITCHES-LONG "  /* needs space at end */

/*
** ISOGEN part table characteristics
*/
#define ISO_CHARX_COMP_ID_STR    "ISOGEN_COMPONENT_ID"
#define ISO_CHARX_SKEY_STR       "ISOGEN_SKEY"
#define MATERIALS                "MATERIALS"
#define CROSS_COMP_ID            "CROSS"
#define ELBOW_COMP_ID            "ELBOW"
#define GASKET_COMP_ID           "GASKET"
#define TEE_COMP_ID              "TEE"
#define VALVE_COMP_ID            "VALVE"
#define INSTRUMENT_COMP_ID       "INSTRUMENT"
#define CAP_COMP_ID              "CAP"
#define MISC_COMP_ID             "MISC-COMPONENT"
#define PIPE_ID					 "PIPE"
#define TUBE_ID					 "TUBE"
#define TEE_SET_ON_ID            "TEE-SET-ON"

using namespace NXOpen;

class pcf_generation
{
public:
	static Session *mySession;
	pcf_generation();
	~pcf_generation();

	int  validate_pcf_file_name( char pcf_file_name[] );
	void handle_status_error( int status );
	int get_part_number_charx(int charx_count,UF_ROUTE_charx_p_t charx_list, tag_t comp_tag, char* out_charx);
	int get_material_charx(int charx_count, UF_ROUTE_charx_p_t charx_list, tag_t comp_tag, char* out_charx);
	int  write_string( char *string, FILE *file_stream );
	int  close_files( FILE * pcf_stream, FILE * material_stream );
	int  create_component_file( tag_t part_tag, char *pcf_name);
	int create_tube_file( tag_t part_tag, tag_t stock_tag, char *pcf_name);
	int  build_header( tag_t part_tag, FILE * file_stream );
	int build_pipeline_ref( tag_t part_tag, FILE * pcf_stream );
	int add_pipeline_attributes( tag_t part_tag, FILE * pcf_stream );
	logical item_exists_in_array(tag_t item,tag_t *array, int num_items);
	int get_tee_set_on_components( tag_t part, int * num_rcps,tag_t ** rcps );
	int get_center_and_branch_points_for_tee_set_on ( tag_t rcp,int charx_count,UF_ROUTE_charx_p_t charx_list, double center_pt[3],double branch_pt[3]);
	int has_part_no_and_material(tag_t comp_tag, int charx_count, UF_ROUTE_charx_p_t charx_list);
	int ensure_bolt_udo ( tag_t part_tag, tag_t udo_tag, tag_t gasket );
	void get_bolt_length ( int charx_count,UF_ROUTE_charx_p_t charx_list,double bolt_diameter,double *length );
	void write_bolts_info ( tag_t part_tag, tag_t comp_tag, int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream, FILE * material_stream, char ***item_code, int *n_items);
	int build_components( tag_t part_tag, FILE * pcf_stream, FILE * material_stream );
	int build_tube_components( tag_t part_tag, tag_t stock_tag, FILE * pcf_stream, FILE * material_stream );
	int get_stock_in_part( tag_t part, int * num_stocks, tag_t ** stocks );
	int write_component_id( int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream );
	int write_end_points( double end_pt1[3], double end_pt2[3], double inDiameter, double outDiameter, FILE * pcf_stream );
	int write_item_code( int charx_count, UF_ROUTE_charx_p_t charx_list, tag_t comp_tag, FILE * pcf_stream, FILE * material_stream, char ***item_code_char, int *n_items);
	int build_materials( int charx_count, UF_ROUTE_charx_p_t charx_list, tag_t comp_tag, FILE * material_stream );
	int write_weight( int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream );
	int write_skey( int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream );
	int ask_comp_anchors(tag_t part_tag, tag_t comp_tag, int *num_anchors, tag_t **anchor_tags);
	int write_center_point( tag_t part_tag, tag_t comp_tag, int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream );
	int write_angle( int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream );
	void find_start_and_end_port ( tag_t* port_tags, tag_t* start_port, tag_t* end_port );
	double find_nps_branch_value ( tag_t* port_tags, double inDiameter );
	int find_nps_branch_index ( int charx_count, UF_ROUTE_charx_p_t charx_list, int* index );
	double determine_in_diameter  ( tag_t comp_tag );
	double determine_out_diameter ( tag_t comp_tag, double inDiameter );
	void write_flat_direction ( tag_t start_port, tag_t end_port, FILE * pcf_stream );
	int write_end_and_branch_points(tag_t part_tag, tag_t comp_tag, FILE * pcf_stream );
	void copy_position(double input_pos[3], double output_pos[3]);
	void write_olet_center_and_branch_points ( tag_t comp_tag, FILE * pcf_stream );
	void write_stub_end_point(tag_t comp_tag, FILE * pcf_stream );
	void get_stub_end_point(tag_t comp_tag);
	void write_flange_blind_point(tag_t part_tag, tag_t comp_tag, FILE * pcf_stream );
	void write_flange_point(tag_t part_tag, tag_t comp_tag, FILE * pcf_stream );
	void write_support_coords ( tag_t part_tag, tag_t comp_tag, FILE * pcf_stream );
	void get_lateral_butt_weld_center_point ( double pnt1[3], double pnt2[3], double vec1[3], double vec2[3], double butt_center[3] );
	void adjust_end_pts_for_tee_set_on ( tag_t  end_rcps[2], double end_pt1[3], double end_pt2[3] );
	void apply_component_attribute ( int counter, tag_t obj_tag, char* title, FILE * pcf_stream );
	void get_comp_attr_titles (void);
	void get_item_attr_titles (void);
	bool check_same_point (double point_1[3], double point_2[3]);
	double line_length (double point_1[3], double point_2[3]);
	void coordinate_transform (double input[3], double output[3]);
	void point_direction (double start_point[3], double end_point[3], double direction[3]);
	void getComponents(std::vector<Assemblies::Component*>& components);

	struct pointinfo
	{
		double end_point[3];
		double diameter;
	};

	struct weldinfo
	{
		double point_1[3];
		double point_2[3];
		double diameter_1;
		double diameter_2;
	};

	std::vector<Assemblies::Component*> components;

private:
	std::vector<pointinfo> control_point;
	std::vector<pointinfo> stubend_point;
	std::vector<weldinfo> additional_weld;
	Part *workPart;
	Part *displayPart;

};


#endif  /* End if UFD_ROUTE_PCF_GENERATION_H_INCLUDED */
