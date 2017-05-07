#include "ufd_route_pcf_generation.hpp"

using namespace std;
using namespace NXOpen;

char pcf_comp_attrs[10][32]= {
    "COMPONENT-ATTRIBUTE1",
    "COMPONENT-ATTRIBUTE2",
    "COMPONENT-ATTRIBUTE3",
    "COMPONENT-ATTRIBUTE4",
    "COMPONENT-ATTRIBUTE5",
    "COMPONENT-ATTRIBUTE6",
    "COMPONENT-ATTRIBUTE7",
    "COMPONENT-ATTRIBUTE8",
    "COMPONENT-ATTRIBUTE9",
    "COMPONENT-ATTRIBUTE10"
 };
int n_pcf_comp_attrs = 10;
char pcf_item_attrs[10][32]= {
    "ITEM-ATTRIBUTE1",
    "ITEM-ATTRIBUTE2",
    "ITEM-ATTRIBUTE3",
    "ITEM-ATTRIBUTE4",
    "ITEM-ATTRIBUTE5",
    "ITEM-ATTRIBUTE6",
    "ITEM-ATTRIBUTE7",
    "ITEM-ATTRIBUTE8",
    "ITEM-ATTRIBUTE9",
    "ITEM-ATTRIBUTE10"
 };
int n_pcf_item_attrs = 10;

#define CHECK_STATUS  if( status != ERROR_OK ) handle_status_error( status );

Session *(pcf_generation::mySession) = NULL;

pcf_generation::pcf_generation()
{
    try
    {
        pcf_generation::mySession = NXOpen::Session::GetSession();
		workPart = mySession->Parts()->Work();
		displayPart = mySession->Parts()->Display();

    }
    catch(exception& ex)
    {
        throw;
    }
}

pcf_generation::~pcf_generation()
{

}


/**********************************************************************
* Function Name: validate_pcf_file_name
*
* Description: This function validates a passed part file name.
*     If the passed filename is not valid, the user is informed and 
*     presented with a dialog to enter a new PCF name.  This is then
*     validated, hence, this function is recursive.
*
* Input: char pcf_file_name[] - the filename to validate.  Includes 
*                               full path information. 
* Output: char pcf_file_nane[] - the constructed pcf file name.  The 
*                                string is empty if an error occurs.
* Returns: ERROR_OK or error code 
***********************************************************************/
int pcf_generation::validate_pcf_file_name( char pcf_file_name[] )
{
   int status = ERROR_OK;
   logical display_dialog = FALSE;

   if( strcmp( pcf_file_name, "" ))
   {
      FILE * test_stream = NULL;

      /* Validate write access for the file.  To test this we simply
         attempt to open the file stream, if we are successful then
         the file is valid and can be used. */
      test_stream = fopen( pcf_file_name,"w+" );

      if( test_stream == NULL )
      {  
         char *messages[3]={ "The file cannot be opened for writing.",
                             "Please make sure the file and directory",
                             "are write accessible." };
         int response = 0;

         UF_UI_message_buttons_t buttons = { TRUE, FALSE, FALSE,
                                             "OK", NULL, NULL,
                                             ERROR_OK,
                                             !(ERROR_OK),
                                             !(ERROR_OK) };
  
         UF_UI_message_dialog( "Invalid Piping Component File",
                               UF_UI_MESSAGE_ERROR,
                               messages,
                               3,
                               FALSE,
                               &buttons,
                               &response );

         display_dialog = TRUE;
         status = 1;
      }
      else
      {
         /* We were able to open a file stream, so all is fine */
         fclose( test_stream );
         status = ERROR_OK;
      }
   }
   else
   {
      /* The pcf file name is empty, we need to prompt the user for
         a valid filename */
      display_dialog = TRUE;
      status = 1;
   }

   /* In the event that the filename is empty or invalid, we need to display
      a file selection dialog box to allow the user to specify a pcf.  If
      the user cancels from this dialog, this application will terminate. */
   if( display_dialog )
   {
      int response = UF_UI_OK;
      char filter_string[ MAX_FSPEC_BUFSIZE ]="*.pcf";

      /* Display the filebox dialog, this will simply return a file
         to create or use.  It will also need to be validated */
      status = UF_UI_create_filebox( "Please specify a Piping Component File",
                                     "Piping Component File Specification",
                                     filter_string,
                                     NULL,
                                     pcf_file_name,
                                     &response );
      CHECK_STATUS
      
      if( response == UF_UI_CANCEL )
      {
         /* The user chose to cancel the file selection operation, there is
            nothing we can do now so just return an error so the main routine
            knows to quit. */
         status = 1;
      }
      else
      {
         status = validate_pcf_file_name( pcf_file_name );

         if( status != ERROR_OK )
         {
            /* There was some sort of error so clear the filename */
            strcpy( pcf_file_name, "" );
         }
      }
   }

   return( status );
}

/**********************************************************************
* Function Name: handle_status_error
*
* Description: This function is used by the CHECK_STATUS macro
*     to display an error message if a non ERROR_OK is returned from
*     from a UG/Open call.
*
* Input: int status - the status code to evaluate 
* Output: None
* Returns: None
***********************************************************************/
void pcf_generation::handle_status_error( int status )
{
  char message[133];
  char err_msg[133];
      
  UF_get_fail_message( status, message );
  
  sprintf( err_msg, "ERROR: %s \n", message );
  
  uc1601( err_msg, 1 ); 
}
/*********************************************************************
 * Function Name: get_part_number_charx
 *
 * Function Description: Look for "PART_NUMBER" charx in the charx list
 * if not found, look for DB_PART_MFKID charx. If out_charx is not NULL than
 * copy the charx value to out_charx. Calling routine ensures enough
 * memory available for string copy. 
 * Returns 0, if charx found else 1
 *
 * INPUT:
 *      charx_list[charx_count] = array of routing characteristics
 *      comp_tag - part occurrence to search attribute on
 * OUTPUT
 *      out_charx - charx value - calling routine supplies enough memory 
*********************************************************************/
int pcf_generation::get_part_number_charx
( 
    int charx_count,
    UF_ROUTE_charx_p_t charx_list,
    tag_t comp_tag,
    char* out_charx
)
{
    int retval =  0;
    int index = 0;

    UF_ROUTE_find_title_in_charx ( charx_count, charx_list, UF_ROUTE_PART_NUMBER_STR, &index );

    if ( index < 0 )
    {
        int is_attr = FALSE;
        UF_ATTR_value_t value;

        value.value.string = NULL;

        UF_ATTR_find_attribute ( comp_tag, UF_ATTR_string, "DB_PART_MFKID", &is_attr );

        if ( !is_attr )
            return 1;

        if ( out_charx != NULL )
        {
            UF_ATTR_read_value ( comp_tag, "DB_PART_MFKID", UF_ATTR_string, &value );
            if ( strcmp ( value.value.string, "" ) )
                sprintf ( out_charx, "%s", value.value.string );
        }
    }
    else if (out_charx != NULL)
    {
        sprintf ( out_charx, "%s", charx_list[index].value.s_value ); 
    }
    return retval;
}
/*********************************************************************
 * Function Name: get_material_charx
 *
 * Function Description: Look for "MATERIAL" charx in the charx list
 * if not found, look for DB_MATERIAL charx. If out_charx is not NULL than
 * copy the charx value to out_charx. Calling routine ensures enough
 * memory available for string copy. 
 * Returns 0, if charx found else 1
 *
 * INPUT:
 *      charx_list[charx_count] = array of routing characteristics
 *      comp_tag - part occurrence to search attribute on
 * OUTPUT
 *      out_charx - charx value - calling routine supplies enough memory 
*********************************************************************/
int pcf_generation::get_material_charx
( 
    int charx_count,
    UF_ROUTE_charx_p_t charx_list,
    tag_t comp_tag,
    char* out_charx
)
{
    int retval =  0;
    int index = 0;

    UF_ROUTE_find_title_in_charx ( charx_count, charx_list, UF_ROUTE_MATERIAL_STR, &index );

    if ( index < 0 )
    {
        int is_attr = FALSE;
        UF_ATTR_value_t value;
        value.value.string = NULL;

        UF_ATTR_find_attribute ( comp_tag, UF_ATTR_string, "DB_MATERIAL", &is_attr );

        if ( !is_attr )
            return 1;

        if ( out_charx != NULL )
        {
            UF_ATTR_read_value ( comp_tag, "DB_MATERIAL", is_attr, &value );
            if ( strcmp ( value.value.string, "" ) )
                sprintf ( out_charx, "%s", value.value.string );
            else if ( strcmp ( value.value.reference, "" ) )
                sprintf ( out_charx, "%s", value.value.reference );
        }
    }
    else
    {
        sprintf ( out_charx, "%s", charx_list[index].value.s_value ); 
    }
    return retval;
}
/**********************************************************************
* Function Name: create_component_file 
* 
* Function Description: Create the Piping Component for the given part.
*     Loop through the part and create PCF entries for all UG/Piping 
*     stock and components along with their material information.  
*     Close the PCF and temporary Material file if any error occurs
*     in building the file.
* 
* Input:
*     part_tag - tag of part used to create Isometric Drawing
*     pcf_name - name of the PCF file to create.
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::create_component_file( tag_t part_tag,
                                  char *pcf_name)
{
   FILE * pcf_stream = NULL;
   FILE * material_stream = NULL;
   char syslog_message[133]= "";
   char material_name[  MAX_FSPEC_BUFSIZE ] = "";

   int status = ERROR_OK;
   
   /*
   ** Open the Piping Component File (*.pcf). The FILE stream will
   ** be passed to file building functions.
   */
   pcf_stream = fopen( pcf_name, "w+" );

   if( pcf_stream == NULL )
   {    
      sprintf( syslog_message, "The pcf: %s could not be opened.\n", pcf_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }
   
   /* 
   ** Create a temporary file to store the material section of the
   ** PCF file.  This is an easy way to create the materials information
   ** at the same time as the component information.
   */
   strcpy( material_name, pcf_name );
   strcat( material_name, TEMP_MAT_EXTENSION );
   
   material_stream = fopen( material_name, "w+" );

   if( material_stream == NULL )
   {
      sprintf( syslog_message, "The material file: %s could not be opened.\n", 
               material_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }
   
   {
      char string[ MAX_LINE_BUFSIZE ]="";

      /*
      **  Write Materials section header to the temporary Material file
      */
      sprintf( string, "%s\n", MATERIALS );
      write_string( string, material_stream);

      /*
      ** Look up user preferences for component attributes
      **/

      get_comp_attr_titles(); 

       /*
      ** Look up user preferences for item attributes
      **/

      get_item_attr_titles(); 
     
      /*
      ** Build the Header section. 
      */
      build_header( part_tag, pcf_stream );

      /*
      ** Build the Pipeline Reference section.  
      */
      build_pipeline_ref( part_tag, pcf_stream);

      /*
      ** Build the Components and Materials
      */
	  getComponents(components);

      build_components( part_tag, pcf_stream, material_stream );
   }

   /*
   ** Close the PCF and Material files and merge the two files.
   ** Delete the material file.
   */
   status = close_files( pcf_stream, material_stream );


   /* Open the PCF for appending data to it */
   pcf_stream = fopen( pcf_name, "a+" );

   if( pcf_stream == NULL )
   {    
      sprintf( syslog_message, "The pcf: %s could not be opened.\n", pcf_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }

   /* Open the Material file to read data from it */
   material_stream = fopen( material_name, "r" );

   if( material_stream == NULL )
   {
      sprintf( syslog_message, "The material file: %s could not be opened.\n", 
               material_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }

   /*
   ** Copy the material data over to the PCF 
   */
   while( !feof( material_stream )) 
   {       
      char buffer[ MAX_LINE_BUFSIZE ] = "";

      fread( buffer, 1, 1, material_stream );

      fwrite( buffer, 1, 1, pcf_stream );
   }
   
   status = close_files( pcf_stream, material_stream );

   /* Delete the temporary Material File */
   if( status == ERROR_OK )
   {
      uc4561 ( material_name, 
               4 /* Text File, no specified extension */ );
   }
    
   return( status ); 
}

int pcf_generation::create_tube_file( tag_t part_tag,
								  tag_t stock_tag,
                                  char *pcf_name)
{
   FILE * pcf_stream = NULL;
   FILE * material_stream = NULL;
   char syslog_message[133]= "";
   char material_name[  MAX_FSPEC_BUFSIZE ] = "";

   int status = ERROR_OK;
   
   /*
   ** Open the Piping Component File (*.pcf). The FILE stream will
   ** be passed to file building functions.
   */
   pcf_stream = fopen( pcf_name, "w+" );

   if( pcf_stream == NULL )
   {    
      sprintf( syslog_message, "The pcf: %s could not be opened.\n", pcf_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }
   
   /* 
   ** Create a temporary file to store the material section of the
   ** PCF file.  This is an easy way to create the materials information
   ** at the same time as the component information.
   */
   strcpy( material_name, pcf_name );
   strcat( material_name, TEMP_MAT_EXTENSION );
   
   material_stream = fopen( material_name, "w+" );

   if( material_stream == NULL )
   {
      sprintf( syslog_message, "The material file: %s could not be opened.\n", 
               material_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }
   
   {
      char string[ MAX_LINE_BUFSIZE ]="";

      /*
      **  Write Materials section header to the temporary Material file
      */
      sprintf( string, "%s\n", MATERIALS );
      write_string( string, material_stream);

      /*
      ** Look up user preferences for component attributes
      **/

      get_comp_attr_titles(); 

       /*
      ** Look up user preferences for item attributes
      **/

      get_item_attr_titles(); 
     
      /*
      ** Build the Header section. 
      */
      build_header( part_tag, pcf_stream );

      /*
      ** Build the Pipeline Reference section.  
      */
      build_pipeline_ref( part_tag, pcf_stream);

      /*
      ** Build the Components and Materials
      */
	  build_tube_components( part_tag, stock_tag, pcf_stream, material_stream );
   }

   /*
   ** Close the PCF and Material files and merge the two files.
   ** Delete the material file.
   */
   status = close_files( pcf_stream, material_stream );


   /* Open the PCF for appending data to it */
   pcf_stream = fopen( pcf_name, "a+" );

   if( pcf_stream == NULL )
   {    
      sprintf( syslog_message, "The pcf: %s could not be opened.\n", pcf_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }

   /* Open the Material file to read data from it */
   material_stream = fopen( material_name, "r" );

   if( material_stream == NULL )
   {
      sprintf( syslog_message, "The material file: %s could not be opened.\n", 
               material_name );
      UF_print_syslog( syslog_message, FALSE );

      return( UF_err_operation_aborted );
   }

   /*
   ** Copy the material data over to the PCF 
   */
   while( !feof( material_stream )) 
   {       
      char buffer[ MAX_LINE_BUFSIZE ] = "";

      fread( buffer, 1, 1, material_stream );

      fwrite( buffer, 1, 1, pcf_stream );
   }
   
   status = close_files( pcf_stream, material_stream );

   /* Delete the temporary Material File */
   if( status == ERROR_OK )
   {
      uc4561 ( material_name, 
               4 /* Text File, no specified extension */ );
   }
    
   return( status ); 
}

int pcf_generation::build_tube_components( tag_t part_tag,
							 tag_t stock_tag,
                             FILE * pcf_stream,
                             FILE * material_stream )
{
   tag_t stock_data = NULL_TAG;
   tag_t *segments = NULL;
   double end_pt1[3];
   double end_pt2[3];
   UF_ROUTE_charx_p_t charx_list;
   int status = ERROR_OK;
   int index = 0;
   int num_segments = 0;
   int charx_count;
   tag_t *rcps = NULL;

   int n_items = 0;

   char **item_code_char = NULL;
   char tso_pcf_string[ MAX_FSPEC_BUFSIZE ] = "";

   
   /*
   ** Get all of the stock and build/write a PCF entry for each valid stock
   ** segment.
   */

   //*****add by CJ********Find the wcs************
	double end_pt1_abs[3];
    double end_pt2_abs[3];
	double label_coordinate[200][3];
	double direction[3];
	int label_number = 0;

    logical is_interior = FALSE;
   
	status = UF_ROUTE_is_stock_interior( stock_tag,
                                        &is_interior );
    CHECK_STATUS
      
    /*
    ** Don't include interior stock in the PCF
    */
    if( !is_interior )
    {
        /*
        ** Get all of the charxs for the stock and find the charx required for
        ** the stock entry in the PCF file      
        */ 
        status = UF_ROUTE_ask_stock_stock_data(stock_tag, &stock_data);
        CHECK_STATUS
         
        status = UF_ROUTE_ask_characteristics ( stock_data,
                                                UF_ROUTE_CHARX_TYPE_ANY,
                                                &charx_count,
                                                &charx_list );
        CHECK_STATUS
         
        status = UF_ROUTE_ask_stock_segments( stock_tag,
                                            &num_segments,
                                            &segments );
        CHECK_STATUS


        for ( int jj = 0; jj < num_segments; jj++ )
        {
            logical is_corner = FALSE;
            int cnr_type      = 0;
            tag_t cnr_rcp     = NULL_TAG;
            tag_t cnr_obj     = NULL_TAG;
            tag_t end_rcps[2] = {NULL_TAG, NULL_TAG};

            status = UF_ROUTE_ask_segment_end_pnts ( segments[jj],
                                                    end_pt1_abs,
                                                    end_pt2_abs );

			//*****add by CJ********Transfer the coordinate to wcs************
			coordinate_transform(end_pt1_abs, end_pt1);
			coordinate_transform(end_pt2_abs, end_pt2);

            CHECK_STATUS
             
            status = UF_ROUTE_ask_seg_rcps ( segments[jj], end_rcps );

            CHECK_STATUS
             
            /* need to adjust the end points for TEE SET ON condition*/
            adjust_end_pts_for_tee_set_on ( end_rcps, end_pt1, end_pt2 );

            /* Check for corner segment, it needs to be treated as bend
            and not pipe, see pcf documentation */
            is_corner = UF_ROUTE_ask_obj_corner_info ( segments[jj],
                                                    &cnr_type,
                                                    &cnr_rcp,
                                                    &cnr_obj );
            /*
            ** Write the component id for the stock
            */
            if ( is_corner  && cnr_type == UF_ROUTE_CORNER_BEND )
            {
                char string[ MAX_LINE_BUFSIZE ] = "";
                 
                strcpy ( string, BEND_PIPE_ID );
                strcat ( string, "\n" );
                status = write_string ( string, pcf_stream );
            }
            else
            {
                write_component_id( charx_count, charx_list, pcf_stream );
            }

            /* determine in and out diameters, then write the end points and diameters for the stock */
            double inDiameter  = determine_in_diameter( stock_data );
            double outDiameter = determine_out_diameter( stock_data, inDiameter );
            write_end_points( end_pt1, end_pt2, inDiameter, outDiameter, pcf_stream ); 
             
            /* For bend corner, write center point, which is same as corner rcp
                position and the SKEY */
            if ( is_corner && cnr_type == UF_ROUTE_CORNER_BEND )
            {
                int err_code = 0;
                double center_pnt_abs[3];
				double center_pnt[3];
                char *pcf_string = NULL;

                UF_ROUTE_ask_rcp_position ( cnr_rcp, center_pnt_abs);
				coordinate_transform(center_pnt_abs, center_pnt);

                pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                                        &err_code );

                sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                        center_pnt[0], center_pnt[1], center_pnt[2]);

                status = write_string( pcf_string, pcf_stream );

                UF_free ( pcf_string );

                /* Bend needs skey. We assume plain end connection, see pcf documentation
                for furter information */
                pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                                        &err_code );

                sprintf ( pcf_string, COMP_SKEY_FMT, BEND_PIPE_SKEY );

                status = write_string( pcf_string, pcf_stream );

                UF_free ( pcf_string );

            }

            /*
            ** Write the item code for the stock and include any unique items
            ** in the material list.
            */
			write_item_code( charx_count, charx_list, stock_tag, pcf_stream,
                            material_stream, &item_code_char, &n_items );

            /*
            ** Write the weight for the stock  
            */
            write_weight( charx_count, charx_list, pcf_stream );

            /* Add COMPONENT-ATTRIBUTE1 - 9 attributes */
            for ( int kk = 0; kk < n_pcf_comp_attrs; kk++ )
            {
                apply_component_attribute( kk, stock_tag, pcf_comp_attrs[kk], pcf_stream ); 
            }
        }

        /*
        ** Free up the routing characteristics and prepare it for use again
        */
        if( charx_count > 0 )
        {
        status = UF_ROUTE_free_charx_array( charx_count,
                                            charx_list );
        CHECK_STATUS

        charx_count = 0;
        charx_list = NULL;
        }
    }

   
   if ( item_code_char != NULL )
   {
      for (int i = 0; i < n_items; i++ )
         UF_free ( item_code_char[i] );

      UF_free ( item_code_char );
   }
   
   return( status );
}


/**********************************************************************
* Function Name: write_string
* 
* Function Description: Write the given string to a file. The function 
*     uses ANSI file stream operations.
*
* Input: char * string - formatted character string
*        FILE * file_stream - the file stream to write to
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_string(char *string,
                        FILE *file_stream )

{
    int num_char = 0;
    int num_write = 0;
    int status = ERROR_OK;

    num_char = strlen( string );
    num_write = fwrite( string, sizeof( char ), num_char, file_stream );

    if( num_write != num_char )
    {
       /* For some reason, the number of characters written to the 
          file_stream do not match the number we should have written */
       UF_print_syslog( "Write operation failure.\n", FALSE );

       status = UF_err_operation_aborted;
       
    }

    return( status );
} 

/**********************************************************************
* Function Name: close_files
* 
* Function Description: Close the PCF and temporary Material files.
*
* Input: pcf_stream - the FILE stream of the PCF
*        material_stream - the FILE stream of the material file
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::close_files( FILE * pcf_stream,
                        FILE * material_stream ) 
{
   int status = ERROR_OK;
   
   if( fclose( pcf_stream ))
   {  
      /* An error occurred closing the pcf_stream */
      UF_print_syslog( "The PCF could not be closed.\n", FALSE );
      status = UF_err_operation_aborted;
   }

   if( fclose( material_stream ))
   {
      /* An error occurred closing the material_stream */
      UF_print_syslog( "The Material File could not be closed.\n", FALSE );
      status = UF_err_operation_aborted;
   }

   return( status );
}

/**********************************************************************
* Function Name: build_header
* 
* Function Description: Build the Header section of the PCF. Use the 
*     default ISOGEN Files file and set the units information based on 
*     the units of the given part.
*
* Input: part_tag - tag of part used to create PCF data
*        FILE * file_stream - the FILE stream of the PCF 
*
* Returns: ERROR_OK or UF_err_operation_aborted
*
***********************************************************************/
int pcf_generation::build_header(tag_t part_tag,
                        FILE * file_stream )

{
   char * pcf_string;
   int status = ERROR_OK;
   int units = 0;
   int err_code = 0;

   /*
   ** Build the Header section.  Use the default ISOGEN Files file
   ** and set the units information based on the units of the input
   ** part.
   */
   pcf_string = (char *) UF_allocate_memory(( ISO_BLOCK_SIZE * MAX_LINE_BUFSIZE ),
                                              &err_code );

   if( err_code )
   {
      UF_print_syslog( "Memory allocation for header failed.\n", FALSE );
      return( UF_err_operation_aborted );
   }

   UF_PART_ask_units( part_tag, &units );

   if( units == UF_PART_ENGLISH )
   {
      sprintf( pcf_string, HEADER_FMT, DEFAULT_ISOGEN_FILES, 
               "INCH", "INCH", "INCH", "INCH", "LBS");
   }
   else
   {
      sprintf( pcf_string, HEADER_FMT, DEFAULT_ISOGEN_FILES, 
               "MM", "MM", "MM", "MM", "KGS");
   }

   status = write_string( pcf_string, file_stream );

   UF_free( pcf_string );

   return( status );
}

/**********************************************************************
 * Function Name: get_comp_attr_titles
 *
 * Function Description: Looks for user preference: "ISOGEN_COMPONENT_ATTR"
 * and stores the string_values in the array pcf_comp_attrs
 *
 * Input:
 * Output: <updates the pcf_comp_attrs with the preference>
 * Returns:
 */
void pcf_generation::get_comp_attr_titles (void)
{
    UF_ROUTE_user_preference_t prefs;

    prefs.key = "ISOGEN_COMPONENT_ATTRIBUTES";
    prefs.type = UF_ROUTE_USER_PREF_TYPE_STR_ARRAY;
    prefs.count = 0;

    UF_ROUTE_ask_user_preferences( 1, &prefs );

    int k = 0;
    for ( int i = 0; i < prefs.count; i++)
    {
        if ( k > 9 )
            break;

        if ( prefs.value.strings[i] == NULL )
            continue;

        strcpy( pcf_comp_attrs[k], prefs.value.strings[i] );
        k++;
    }
    
    if( prefs.count > 0 )
        UF_ROUTE_free_user_prefs_data( 1, &prefs );

    n_pcf_comp_attrs = k;
}

/**********************************************************************
 * Function Name: get_item_attr_titles
 *
 * Function Description: Looks for user preference: "ISOGEN_COMPONENT_ATTR"
 * and stores the string_values in the array pcf_comp_attrs
 *
 * Input:
 * Output: <updates the pcf_comp_attrs with the preference>
 * Returns:
 */
void pcf_generation::get_item_attr_titles (void)
{
    UF_ROUTE_user_preference_t prefs;

    prefs.key = "ISOGEN_ITEM_ATTRIBUTES";
    prefs.type = UF_ROUTE_USER_PREF_TYPE_STR_ARRAY;
    prefs.count = 0;

    UF_ROUTE_ask_user_preferences( 1, &prefs );

    int k = 0;
    for ( int i = 0; i < prefs.count; i++)
    {
        if ( k > 9 )
            break;

        if ( prefs.value.strings[i] == NULL )
            continue;

        strcpy( pcf_item_attrs[k], prefs.value.strings[i] );
        k++;
    }
    
    if( prefs.count > 0 )
        UF_ROUTE_free_user_prefs_data( 1, &prefs );

    n_pcf_item_attrs = k;
}
/**********************************************************************
* Function Name: add_pipeline_attributes
* 
* Function Description: Hamworthy Gas Systems custom enhacement:
*  HSG needs set of attributes to be added to
*  pipeline reference. Attribute titles are 
*  ATTRIBUTE1, ATTRIBUTE2..... ATTRIBUTE51
*  HSG will add the values to these attributes
*  based on their site requirements. 
*
* Input: part_tag - part for Isometric Drawing
*        FILE *pcf_stream - FILE stream for the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::add_pipeline_attributes( tag_t part_tag,
                               FILE * pcf_stream )
{
    int i = 0;
    int status = 0;
    UF_ATTR_value_t value;
    

    for ( i = 0; i < 99; i++ )
    {
        char attr_title[ MAX_LINE_BUFSIZE ]="";
        char pcf_string[ MAX_LINE_BUFSIZE ]="";
        char attr_value[ MAX_LINE_BUFSIZE ]="";

        sprintf ( attr_title, "%s%d", "ATTRIBUTE", i+1 );

        UF_ATTR_read_value ( part_tag, attr_title, UF_ATTR_any, &value );
         
        switch ( value.type )
        {
            case UF_ATTR_integer:
                sprintf ( attr_value, "%d", value.value.integer );
                sprintf ( pcf_string, "    %s    %s\n", attr_title, attr_value );
                status = write_string( pcf_string, pcf_stream );
                break;
            case UF_ATTR_real:
                sprintf ( attr_value, "%.4f", value.value.real );
                sprintf ( pcf_string, "    %s    %s\n", attr_title, attr_value );
                status = write_string( pcf_string, pcf_stream );
                break;
            case UF_ATTR_string:
                sprintf ( attr_value, "%s", value.value.string );
                sprintf ( pcf_string, "    %s    %s\n", attr_title, attr_value );
                status = write_string( pcf_string, pcf_stream );
                break;
            default:
                break;
        }
    }
    return status;
}

/**********************************************************************
* Function Name: build_pipeline_ref
* 
* Function Description: Build the Pipeline Reference section of the PCF.
*
* Input: part_tag - part for Isometric Drawing
*        FILE *pcf_stream - FILE stream for the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::build_pipeline_ref( tag_t part_tag,
                               FILE * pcf_stream )
{
   char pipeline_name[ MAX_FSPEC_BUFSIZE ];
   char *pcf_string = NULL;
   int status = ERROR_OK;
   char date_buffer[128] = "";
   int err_code = ERROR_OK;
   time_t ltime;
   struct tm *today;

   UF_ATTR_info_t MLI_info;
   UF_ATTR_info_t REVISION_info;
   logical has_attr = false;
   char additional_string[100];
   char attribute_string[200];

   /*
   ** Build the Pipeline Reference section.  Use the input part name as the
   ** pipeline name and the system date for the date.  The month values run 
   ** from 0 - 11...add 1 to get the true month number.
   */
   status =  UF_PART_ask_part_name( part_tag,
                                    pipeline_name );
   CHECK_STATUS

   status = uc4574 ( pipeline_name, 
                     2, /* Strip off the .prt extension */
                     pipeline_name );
   CHECK_STATUS
   
   
   pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                             &err_code );
   if( err_code )
   {
      UF_print_syslog( "Memory allocation for pipiline reference failed.\n",
                       FALSE );
      return( UF_err_operation_aborted );
   }

   sprintf( pcf_string, PIPELINE_REF_FMT, pipeline_name );
   status = write_string( pcf_string, pcf_stream );

   /* Get the current date using the ANSI date functionality */
   ltime = time (NULL );
   today = localtime( &ltime );

   strftime( date_buffer, 128,
        "%d/%m/%y", today );

   sprintf( pcf_string, DATE_FMT, date_buffer );

   status = write_string( pcf_string, pcf_stream );

   UF_ATTR_get_user_attribute_with_title_and_type(part_tag, "MLI", UF_ATTR_string, UF_ATTR_NOT_ARRAY, &MLI_info, &has_attr);
   if(has_attr)
   {
	   sprintf(additional_string,"    PROJECT-IDENTIFIER  MLI %s\n", MLI_info.string_value);
	   status = write_string( additional_string, pcf_stream );
	   memset(additional_string,0,sizeof(additional_string)/sizeof(char));
	   has_attr = false;
	   UF_ATTR_free_user_attribute_info_strings(&MLI_info);
   }
   UF_ATTR_get_user_attribute_with_title_and_type(part_tag, "REVISION", UF_ATTR_string, UF_ATTR_NOT_ARRAY, &REVISION_info, &has_attr);
   if(has_attr)
   {
	   sprintf(additional_string,"    REVISION            %s\n", REVISION_info.string_value);
	   status = write_string( additional_string, pcf_stream );
	   memset(additional_string,0,sizeof(additional_string)/sizeof(char));
	   has_attr = false;
	   UF_ATTR_free_user_attribute_info_strings(&REVISION_info);
   }

	NXOpen::Annotations::NoteCollection::iterator it_note;
	for(it_note = workPart->Notes()->begin() ; it_note != workPart->Notes()->end() ; it_note ++ )				
	{
		Annotations::Note* note = (Annotations::Note*) *it_note;
		if(note != NULL)
		{
			std::vector<NXString> title = note->GetText();
			if(title.size()> 0)
			{
				//char *title_str = new char[strlen(title[0].GetLocaleText())+1];
				//strcpy(title_str, title[0].GetLocaleText());
				if(strcmp(title[0].GetLocaleText(),"NOTES:")== 0)
				{
					int count = 0;
					for(int k=1; k<title.size(); k++)
					{
						if(strlen(title[k].GetLocaleText())>0)
						{
							sprintf(attribute_string,"    ATTRIBUTE%d         %s\n", count+10, title[k].GetLocaleText());
							status = write_string( attribute_string, pcf_stream );
							count++;
						}
					}
				}
			}
		}
	}


   UF_free( pcf_string );


   /* add pipeline attributes */
   add_pipeline_attributes ( part_tag, pcf_stream );
   return( status );
}

/*==========================================================================
 * item_exists_in_array
 * Description:
 *  Checks wheter the input item already exists in the input array
 *
 *  Input:
 *      tag_t item   - the item to check
 *      tag_t* array[num_items] - existing item array
 *  Returns:
 *      TRUE if the item exists
 */
logical pcf_generation::item_exists_in_array
(
    tag_t item,
    tag_t *array,
    int num_items
)
{
    int ii = 0;
    logical retval = FALSE;

    for ( ii = 0; ii < num_items; ii++ )
    {
        if ( array[ii] == item )
        {
            retval = TRUE;
            break;
        }
    }
    return retval;
}
/*==========================================================================
  Function: get_stock_in_part                                    
  Description:  This function finds and returns to the user all of the 
     stock found in the passed part.  Typical use would involved passing in
     the work part tag - but any part tag could be passed.  Note that stock
     is found only at the top level of the part.  Any stock in subcomponents
     of the passed part tag are not returned!
                                                                          
  Input:  tag_t part - the part tag to search for stock    
  Output: int * num_stocks - the number of stocks found in the part
          tag_t ** stocks - the array of stock tags found - must be freed 
                            by the user with UF_free
  Return: ERROR_OK or error_code                                         
**==========================================================================*/
int pcf_generation::get_stock_in_part( tag_t part, 
                              int * num_stocks,
                              tag_t ** stocks )
{
   int error_code = 0;
   tag_t object = NULL_TAG;

   do
   {
      UF_OBJ_cycle_objs_in_part( part, 
                                 UF_route_stock_type, 
                                 &object );

      if ( object != NULL_TAG && !item_exists_in_array ( object, stocks[0], *num_stocks ))
	  //**********Add by CJ*********** Assembly ***********
	  //if ( object != NULL_TAG && !item_exists_in_array ( object, stocks[0], *num_stocks ) && !UF_ASSEM_is_occurrence (object))
      {
         (*num_stocks)++;
         (*stocks) = (tag_t *)UF_reallocate_memory( *stocks, 
                                                    ((*num_stocks) * sizeof( tag_t )), 
                                                    &error_code );

         if(( *stocks == NULL ) ||
            ( error_code != ERROR_OK ))
         {
            *num_stocks = 0;
            return( 1 );
         }

         (*stocks)[(*num_stocks) - 1] = object;
      
      }
   }while( object != NULL_TAG );

   do
   {
      tag_t stock_tag = NULL_TAG;

      UF_OBJ_cycle_objs_in_part( part, 
                                 UF_component_type, 
                                 &object );
      
      if ( object != NULL_TAG )
          UF_ROUTE_ask_object_stock ( object, &stock_tag );

      if ( stock_tag != NULL_TAG && 
           !item_exists_in_array ( stock_tag, stocks[0], *num_stocks ) )
      {
         (*num_stocks)++;
         (*stocks) = (tag_t *)UF_reallocate_memory( *stocks, 
                                                    ((*num_stocks) * sizeof( tag_t )), 
                                                    &error_code );

         if(( *stocks == NULL ) ||
            ( error_code != ERROR_OK ))
         {
            *num_stocks = 0;
            return( 1 );
         }

         (*stocks)[(*num_stocks) - 1] = stock_tag;
      
      }
   }while( object != NULL_TAG );
       
   return( ERROR_OK );
}

/*==========================================================================
  get_lateral_butt_weld_center_point                                    
  Description:  Given two points, and vectors at each point, this function
               determines the intersection point between the two vectors. 
               The calling routine needs to make sure that the vectors are 
               not parallel. That way, there is always a solution
                                                                          
  Input:  pnt1[3] - first point
          pnt2[3] - second point
          vec1[3]    - vector at first point
          vec2[3]    - vector at second point
  
  Output: butt_center[3]   - point of intersection of above two vectors
                               
Note: Initially this routine was needed only for lateral butt weld parts 
hence the name. Now, this is a generic routine used frequently.
**==========================================================================*/
void pcf_generation::get_lateral_butt_weld_center_point ( double pnt1[3], 
                                                 double pnt2[3], 
                                                 double vec1[3], 
                                                 double vec2[3], 
                                                 double butt_center[3] )
{
    double   mag = 0.0,
             v1_dot_v1 = 0.0, v1_dot_v2 = 0.0, 
             v2_dot_v2 = 0.0, p1_dot_v1 = 0.0, 
             p1_dot_v2 = 0.0, p2_dot_v1 = 0.0, 
             p2_dot_v2 = 0.0, denom     = 0.0,
             sum1      = 0.0, sum2      = 0.0;
    double   tval1     = 0.0, tval2     = 0.0;
	double	 vec_1[3] = {0}, vec_2[3] = {0};

    UF_VEC3_negate ( vec1, vec_1 );
    UF_VEC3_negate ( vec2, vec_2 );

    UF_VEC3_unitize ( vec_1, 0.001, &mag, vec_1 );
    UF_VEC3_unitize ( vec_2, 0.001, &mag, vec_2 );

    UF_VEC3_dot(vec_1, vec_1, &v1_dot_v1);
    UF_VEC3_dot(vec_2, vec_2, &v2_dot_v2);
    UF_VEC3_dot(vec_1, vec_2, &v1_dot_v2);
    UF_VEC3_dot(pnt1, vec_1, &p1_dot_v1);
    UF_VEC3_dot(pnt1, vec_2, &p1_dot_v2);
    UF_VEC3_dot(pnt2, vec_1, &p2_dot_v1);
    UF_VEC3_dot(pnt2, vec_2, &p2_dot_v2);

    denom = (v1_dot_v1 * v2_dot_v2 - v1_dot_v2 * v1_dot_v2);

    if (fabs(denom) > 1.0e-10)
    {
        sum1 = -p1_dot_v1 + p2_dot_v1;
        sum2 = p1_dot_v2 - p2_dot_v2;
        tval1 = (sum1 * v2_dot_v2 + v1_dot_v2 * sum2) / denom;
        tval2 = (v1_dot_v1 * sum2 + sum1 * v1_dot_v2) / denom;
    }
    else
    {
        if (fabs(v2_dot_v2) > 1.0e-6)
        {
            tval1 = 0.0;
            tval2 = (p1_dot_v2 - p2_dot_v2) / (v2_dot_v2 * v2_dot_v2);
        }
        else if (fabs(v1_dot_v1) > 1.0e-6)
        {
            tval1 = (p1_dot_v1 + p2_dot_v1) / (v1_dot_v1 * v1_dot_v1);
            tval2 = 0.0;
        }
        else
        {
            tval1 = 0.0;
            tval2 = 0.0;
        }
    }

    if ( (fabs ( tval1 - 0.0 ) ) < 0.0001 ) 
        UF_VEC3_affine_comb ( pnt2, tval2, vec_2, butt_center );
    else
        UF_VEC3_affine_comb ( pnt1, tval1, vec_1, butt_center );

    return;
}
/*==========================================================================
  Function: get_tee_set_on_components                                    
  Description:  This function finds and returns to the user all of the 
     rcps which satisfy TEE_SET_ON condition: 1) RCP must be referenced
     by three segments, 2) Three segments must form a 'T', 3) rcp must
     have TEE_SET_ON attribute defined. 
                                                                          
  Input:  tag_t part - the part tag to search for stock    
  Output: int * num_rcps - the number of rcps found in the part
          tag_t ** rcps - the array of stock tags found - must be freed 
                            by the user with UF_free
  Return: ERROR_OK or error_code                                         
**==========================================================================*/
int pcf_generation::get_tee_set_on_components( tag_t part, 
                              int * num_rcps,
                              tag_t ** rcps )
{
   int   error_code = 0;
   int   tee_set_on = FALSE;
   tag_t object     = NULL_TAG;

   do
   {
      UF_OBJ_cycle_objs_in_part( part, 
                                 UF_route_control_point_type, 
                                 &object );

      if ( object != NULL_TAG )
      {
          int   nsegs       = 0;
          int   status      = 0;
          int   index       = 0;
          int   charx_count = 0;
          tag_t *segs       = NULL;
          UF_ROUTE_charx_p_t charx_list;
          
          UF_ROUTE_ask_rcp_segs ( object, &nsegs, &segs );
         
          if ( nsegs != 3 )
          {  
             continue;
          }

          UF_free ( segs );  
          
          status = UF_ROUTE_ask_characteristics( object, 
                                             UF_ROUTE_CHARX_TYPE_ANY,
                                             &charx_count, 
                                             &charx_list);

          if ( !charx_count || status != ERROR_OK)
             continue;

          status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                             ISO_CHARX_COMP_ID_STR, 
                                             &index);


          if( charx_list[index].value.s_value != NULL &&
              strcmp( charx_list[index].value.s_value, TEE_SET_ON_ID ) == 0 )
              tee_set_on = TRUE;

           if( charx_count > 0 )
               status = UF_ROUTE_free_charx_array( charx_count,
                                                   charx_list );
      }
      if ( tee_set_on )
      {
         (*num_rcps)++;
         (*rcps) = (tag_t *)UF_reallocate_memory( *rcps, 
                                                    ((*num_rcps) * sizeof( tag_t )), 
                                                    &error_code );

         if(( *rcps == NULL ) ||
            ( error_code != ERROR_OK ))
         {
            *num_rcps = 0;
            return( 1 );
         }

         (*rcps)[(*num_rcps) - 1] = object;
         
         tee_set_on = FALSE;
      }
   }while( object != NULL_TAG );
       
   return( ERROR_OK );
}



/**********************************************************************
* Function Name: get_center_and_branch_points_for_tee_set_on
* 
* Function Description: Obtain the center points and any branch points for 
*     the given rcp.
*
* Input: tag_t rcp      -  rcp to work on
*        charx_count    -  number of charx on the rcp
*        charx_list     -  array of charx
*
* Output: center_pt     -  rcp position
*         branch_pt     -  rcp position + half run len in appropriate 
*                          direction
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::get_center_and_branch_points_for_tee_set_on ( tag_t rcp,
                                         int charx_count,
                                         UF_ROUTE_charx_p_t charx_list, 
                                         double center_pt[3],
                                         double branch_pt[3])
{
    int     nsegs       = 0,
            index       = 0,
            found_attr  = 0, i = 0;
    tag_t   *segs       = 0;
    double  center[3]   = {0.0, 0.0, 0.0},
            end1[3]     = {0.0, 0.0, 0.0},
            end2[3]     = {0.0, 0.0, 0.0},
            end3[3]     = {0.0, 0.0, 0.0},
            branch[3]   = {0.0, 0.0, 0.0};
    double vec1[3], vec2[3], vec3[3], dot, mag;
    tag_t   far_rcps[3]= {NULL_TAG, NULL_TAG, NULL_TAG};

    UF_ATTR_find_attribute ( rcp, UF_ATTR_any, "R$HALF_RUN_LEN", &found_attr );
    if ( !found_attr )
        UF_ATTR_find_attribute ( rcp, UF_ATTR_any, "HALF_RUN_LEN", &found_attr );
    
    if ( !found_attr )
        return 1;
    else
        UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                      "HALF_RUN_LEN", &index );
    
    UF_ROUTE_ask_rcp_segs ( rcp, &nsegs, &segs );
    
    for ( i = 0; i < nsegs; i++ )
    {
        tag_t seg_rcps[2] = {NULL_TAG, NULL_TAG};
        UF_ROUTE_ask_seg_rcps ( segs[i], seg_rcps );

        if ( seg_rcps[0] == rcp )
            far_rcps[i] = seg_rcps[1];
        else
            far_rcps[i] = seg_rcps[0];
    }
    UF_free ( segs );

    UF_ROUTE_ask_rcp_position ( rcp, center );
    UF_ROUTE_ask_rcp_position ( far_rcps[0], end1 );
    UF_ROUTE_ask_rcp_position ( far_rcps[1], end2 );
    UF_ROUTE_ask_rcp_position ( far_rcps[2], end3 );

    for ( i =0; i < 3; i++ )
    {
        vec1[i] = center[i] - end1[i];
        vec2[i] = center[i] - end2[i];
        vec3[i] = center[i] - end3[i];
    }
    
    UF_VEC3_unitize ( vec1, 0.0001, &mag, vec1 );
    UF_VEC3_unitize ( vec2, 0.0001, &mag, vec2 );
    UF_VEC3_unitize ( vec3, 0.0001, &mag, vec3 );

    UF_VEC3_dot ( vec1, vec2, &dot );
    if ( dot < -0.9 ) /* 1 & 2 are opposite */
    {
        UF_VEC3_affine_comb ( center, (-1.0) * charx_list[index].value.r_value,
                              vec3, branch );
    }
    else
    {
        UF_VEC3_dot ( vec1, vec3, &dot );
        if ( dot < -0.9 ) /* 1 & 3 are opposite */
        {
            UF_VEC3_affine_comb ( center, (-1.0) * charx_list[index].value.r_value,
                                  vec2, branch );
        }
        else /* 2 & 3 are opposite */
        {
            UF_VEC3_affine_comb ( center, (-1.0) * charx_list[index].value.r_value,
                                  vec1, branch );
        }
    }

    for ( i = 0; i < 3; i++ )
    {
        center_pt[i] = center[i];
        branch_pt[i] = branch[i];
    }

    return ERROR_OK;
}

/****************************************************************************
* adjust_end_pts_for_tee_set_on
* DESCRIPTION: Check if the rcps are tee_set_on rcps, in which case adjust the
*              end point to account for HALF_RUN_LEN of the welded TEE. 
*
* INPUT:
*    end_rcps = rcps to checak
*    end_pt1  ( I/O ) - current position/adjusted position for rcp1
*    end_pt2  ( I/O ) - current position/adjusted position for rcp2
*/
void pcf_generation::adjust_end_pts_for_tee_set_on 
( 
    tag_t  end_rcps[2], 
    double end_pt1[3], 
    double end_pt2[3] )
{
    int    charx_count    = 0, 
           index          = 0;
    double dot            = 0.0;
    double rcp_pos[3]     = {0.0, 0.0, 0.0};
    double branch_pos[3]  = {0.0, 0.0, 0.0};
    double rcp_vec_dir[3] = {0.0, 0.0, 0.0};
    double tee_vec_dir[3] = {0.0, 0.0, 0.0};
    UF_ROUTE_charx_p_t charx_list = NULL;
    
    UF_ROUTE_ask_characteristics( end_rcps[0], 
                                  UF_ROUTE_CHARX_TYPE_ANY,
                                  &charx_count, 
                                  &charx_list);

    if ( charx_count )
    {
        UF_ROUTE_find_title_in_charx( charx_count, charx_list,
            ISO_CHARX_COMP_ID_STR, 
            &index);


        if( index > -1  &&
            strcmp( charx_list[index].value.s_value, TEE_SET_ON_ID ) == 0 )
        {
            get_center_and_branch_points_for_tee_set_on ( end_rcps[0], 
                                                          charx_count, charx_list,
                                                          rcp_pos, branch_pos );

            tee_vec_dir[0] = rcp_pos[0] - branch_pos[0];
            tee_vec_dir[1] = rcp_pos[1] - branch_pos[1];
            tee_vec_dir[2] = rcp_pos[2] - branch_pos[2];

            rcp_vec_dir[0] = end_pt1[0] - end_pt2[0];
            rcp_vec_dir[1] = end_pt1[1] - end_pt2[1];
            rcp_vec_dir[2] = end_pt1[2] - end_pt2[2];
            
            UF_VEC3_dot ( rcp_vec_dir, tee_vec_dir, &dot );

            if ( rcp_pos[0]- end_pt1[0] < 0.001 &&
                 rcp_pos[1]- end_pt1[1] < 0.001 &&
                 rcp_pos[2]- end_pt1[2] < 0.001 &&
                 ( dot < -0.99 || dot > 0.99 )     )
            {
                end_pt1[0] = branch_pos[0];    
                end_pt1[1] = branch_pos[1];    
                end_pt1[2] = branch_pos[2];    
            }
            else if ( rcp_pos[0]- end_pt2[0] < 0.001 &&
                      rcp_pos[1]- end_pt2[1] < 0.001 &&
                      rcp_pos[2]- end_pt2[2] < 0.001 &&
                      ( dot < -0.99 || dot > 0.99 )     )
            {
                end_pt2[0] = branch_pos[0];    
                end_pt2[1] = branch_pos[1];    
                end_pt2[2] = branch_pos[2];    
            }
        }
    }

    if (charx_count)
        UF_ROUTE_free_charx_array ( charx_count, charx_list );
    
    /* repeat the procedure for next rcp*/
    UF_ROUTE_ask_characteristics( end_rcps[1], 
                                  UF_ROUTE_CHARX_TYPE_ANY,
                                  &charx_count, 
                                  &charx_list);

    if ( charx_count )
    {
        UF_ROUTE_find_title_in_charx( charx_count, charx_list,
            ISO_CHARX_COMP_ID_STR, 
            &index);


        if( index > -1  &&
            strcmp( charx_list[index].value.s_value, TEE_SET_ON_ID ) == 0 )
        {
            get_center_and_branch_points_for_tee_set_on ( end_rcps[1], 
                                                          charx_count, charx_list,
                                                          rcp_pos, branch_pos );

            tee_vec_dir[0] = rcp_pos[0] - branch_pos[0];
            tee_vec_dir[1] = rcp_pos[1] - branch_pos[1];
            tee_vec_dir[2] = rcp_pos[2] - branch_pos[2];

            rcp_vec_dir[0] = end_pt1[0] - end_pt2[0];
            rcp_vec_dir[1] = end_pt1[1] - end_pt2[1];
            rcp_vec_dir[2] = end_pt1[2] - end_pt2[2];
            
            UF_VEC3_dot ( rcp_vec_dir, tee_vec_dir, &dot );

            if ( rcp_pos[0]- end_pt1[0] < 0.001 &&
                 rcp_pos[1]- end_pt1[1] < 0.001 &&
                 rcp_pos[2]- end_pt1[2] < 0.001 &&
                 ( dot < -0.99 || dot > 0.99 )     )
            {
                end_pt1[0] = branch_pos[0];    
                end_pt1[1] = branch_pos[1];    
                end_pt1[2] = branch_pos[2];    
            }
            else if ( rcp_pos[0]- end_pt2[0] < 0.001 &&
                      rcp_pos[1]- end_pt2[1] < 0.001 &&
                      rcp_pos[2]- end_pt2[2] < 0.001 &&
                      ( dot < -0.99 || dot > 0.99 )     )
            {
                end_pt2[0] = branch_pos[0];    
                end_pt2[1] = branch_pos[1];    
                end_pt2[2] = branch_pos[2];    
            }
        }
    }

    if (charx_count)
        UF_ROUTE_free_charx_array ( charx_count, charx_list );
}

/**********************************************************************
* Function Name: has_part_no_and_material
* 
* Function Description
* Checks whether the given component has part number and material 
* characteristics. If not, the component is ignored
*
* <shahk>MATERIAL charx is optional. Check for NPS charx instead
*
* Input: charx_list[charx_count]  -  characteristics 
*    
* Returns: ERROR_OK or 1
*/
int pcf_generation::has_part_no_and_material
(
    tag_t comp_tag,
    int charx_count,
    UF_ROUTE_charx_p_t charx_list
)
{
    int retval = 0;
    int index = 0;

    retval = get_part_number_charx ( charx_count, charx_list, comp_tag, NULL );

    if ( retval )
    {
        char syslog_message[133]="";

        sprintf( syslog_message, 
                 "Failed to find part number characteristics for component: %u \n", 
                 comp_tag );
        UF_print_syslog( syslog_message,    
                         FALSE );

        return 1;
    }

    /*retval = get_material_charx ( charx_count, charx_list, comp_tag, NULL );

    if ( retval )
    {
        char syslog_message[133]="";

        sprintf( syslog_message, 
                 "Failed to find material characteristics for component: %u \n", 
                 comp_tag );
        UF_print_syslog( syslog_message,    
                         FALSE );

        return 1;
    }*/
    
    UF_ROUTE_find_title_in_charx ( charx_count, charx_list, UF_ROUTE_NPS_STR, &index );

    if ( index < 0 )
    {
        char syslog_message[133]="";

        sprintf( syslog_message, 
                 "Failed to find NPS for component: %u \n", 
                 comp_tag );
        UF_print_syslog( syslog_message,    
                         FALSE );

        return 1;
    }

    return ERROR_OK;
}

/**********************************************************************
 * Function Name: ensure_bolt_udo
 * 
 * Function Description: Checks if the UDO is a valid bolt udo for a given
 * gasket. A UDO is valid bolt udo, if its class name is "ROUTING_BOLT"
 * and the associated object of the UDO (which is a ROUTE_connection)
 * is a connection of the given gasket
 *
 * INPUT:
 *    part_tag  -  current part
 *    udo_tag   -  UDO to query
 *    gasket    -  gasket 
 *
 * RETURNS:
 *  1  -  If valid UDO
 *  0  -  If invalid 
 */
int pcf_generation::ensure_bolt_udo ( tag_t part_tag, tag_t udo_tag, tag_t gasket )
{
    int is_valid = FALSE;
    tag_t connection = NULL_TAG;
    UF_UDOBJ_all_data_t all_data;

    tag_t ports[2] ={NULL_TAG, NULL_TAG};
    tag_t part_occ[2] ={NULL_TAG, NULL_TAG};

    UF_UDOBJ_ask_udo_data ( udo_tag, &all_data );

    if ( !all_data.num_links )
    {
        UF_UDOBJ_free_udo_data ( &all_data );
        return FALSE;
    }

    do
    {
        UF_OBJ_cycle_objs_in_part ( part_tag, UF_route_connection_type, &connection );

        if ( connection == all_data.link_defs[0].assoc_ug_tag )
        {
            break;
        }
    }while ( connection != NULL_TAG );
    
    UF_ROUTE_ask_connection_ports ( connection, ports );

    UF_ROUTE_ask_port_part_occ ( ports[0], &part_occ[0] );
    UF_ROUTE_ask_port_part_occ ( ports[1], &part_occ[1] );

    if ( part_occ[0] == gasket || 
         part_occ[1] == gasket )
    {
        is_valid = TRUE;
    }

    UF_UDOBJ_free_udo_data ( &all_data );

    return is_valid;
}
/*********************************************************************
 * get_bolt_length
 *
 * Bolt length = 2* (flange_thickness +  flange_face_depth + flange_tol) 
 *                + bolt_dia + gasket thickness
 *
 * Use the charx on gasket to find the values for above parameters
 */
 void pcf_generation::get_bolt_length ( int charx_count,
                               UF_ROUTE_charx_p_t charx_list,
                               double bolt_diameter,
                               double *length )
 {
     int index = 0;
     double flange_thickness = 0.0;
     double flange_face_dep  = 0.0;
     double flange_tol       = 0.0;
     double gasket_thickness = 0.0;

     UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "FLANGE_THK", &index );
     if ( index > -1 )
     {
         flange_thickness = charx_list[index].value.r_value;
     }
     UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "FACE_DEP", &index );
     if ( index > -1 )
     {
         flange_face_dep = charx_list[index].value.r_value;
     }
     UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "FLANGE_TOL", &index );
     if ( index > -1 )
     {
         flange_tol = charx_list[index].value.r_value;
     }
     UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "GASKET_THK", &index );
     if ( index > -1 )
     {
         gasket_thickness = charx_list[index].value.r_value;
     }

     *length = 2* (flange_thickness +  flange_face_dep + flange_tol) + bolt_diameter + gasket_thickness;
 }
   

/**********************************************************************
* Function Name:  write_bolts_info
*
* Function Description: Write bolt/stud and nuts info for GASKETS
* Bolts and Nuts are UDOs. Cycle through all UDOs to find the bolt/stud
* UDO. Bolts can be added separately or as a part of GASKET entry in PCF
* We will use the later. Following is the format
*
*     BOLT 
*     BOLT-DIA
*     BOLT-LENGTH
*     BOLT-QUANTITY
*     BOLT-ITEM-CODE 
*
* BOLT-DIA and BOLT-QUANTITY are charx on Gasket. BOLT length can be found
* using 2* (flange_thickness +  flange_face_depth + flange_tol) + bolt_dia + gasket thickness
* 
*/
void pcf_generation::write_bolts_info 
( 
    tag_t part_tag,      /* Work part */
    tag_t comp_tag,      /* Gasket */ 
    int charx_count,      
    UF_ROUTE_charx_p_t charx_list, /*Gasket Charx */
    FILE * pcf_stream, 
    FILE * material_stream,
    char ***item_code, /* Array of existing item codes */
    int *n_items       /* total number of unique items */
)
{
    int status = 0;
    tag_t  udo_tag = NULL_TAG;

   do
   {
        logical item_code_is_unique = TRUE;
        int is_bolt = 0;
        int ii = 0, err_code = 0;
        int bolt_charx_count = 0;
        int index = 0;
        int num_items = 0;
        double length = 0.0;

        UF_ROUTE_charx_p_t bolt_charx_list = NULL;

        char pcf_string[ MAX_LINE_BUFSIZE ] = "";
        char bolt_item_code[ MAX_LINE_BUFSIZE ] = "";

        status = UF_OBJ_cycle_objs_in_part( part_tag, 
                                            UF_user_defined_object_type,
                                            &udo_tag );
        
        if ( udo_tag == NULL_TAG )
        {
            return;
        }
        
        /* Find the associated linked objects for this UDO. If the associated
         * linked object is a route connection, then find the ports of the 
         * connection. One of two connected ports should be a port on the
         * gasket. If the condition satisfies, then we found the UDO tag
         * for the bolts/stud. 
         */

        is_bolt = ensure_bolt_udo ( part_tag, udo_tag, comp_tag );

        /* By this time all the gasket information is already written to
         * the PCF stream. Add BOLT info in PCF format:
         * BOLT-ITEM-CODE is the part number of the bolt which is a charx on 
         * bolt UDO. We also write the part number, material and description charx
         * to the materials stream. 
         */
        if ( !is_bolt )
        {
            continue;
        }
         
         sprintf ( pcf_string, "    %s\n", "BOLT");
         write_string ( pcf_string, pcf_stream );
         strcpy ( pcf_string, "" );
         
         UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "BOLT_DIA", &index );
         if ( index > -1 )
         {
             sprintf ( pcf_string, "    %s  %.4f\n", "BOLT-DIA", charx_list[index].value.r_value );
             write_string ( pcf_string, pcf_stream );
             strcpy ( pcf_string, "" );
         }
         
         get_bolt_length ( charx_count, charx_list, charx_list[index].value.r_value, &length );
         sprintf ( pcf_string, "    %s  %.4f\n", "BOLT-LENGTH",  length );
         write_string ( pcf_string, pcf_stream );
         strcpy ( pcf_string, "" );

         UF_ROUTE_find_title_in_charx ( charx_count, charx_list, "NUM_BOLTS", &index );
         if ( index > -1 )
         {
             sprintf ( pcf_string, "    %s  %d\n", "BOLT-QUANTITY", charx_list[index].value.i_value );
             write_string ( pcf_string, pcf_stream );
             strcpy ( pcf_string, "" );
         }

         /* Bolt Charx */
         UF_ROUTE_ask_characteristics ( udo_tag, UF_ROUTE_CHARX_TYPE_STR, 
                                        &bolt_charx_count, &bolt_charx_list );

         UF_ROUTE_find_title_in_charx ( bolt_charx_count, bolt_charx_list, "PART_NUMBER", &index );

         if ( index > -1 )
         {
             if ( strcmp ( bolt_charx_list[index].value.s_value, "" ) )
             {
                 sprintf ( pcf_string, "    %s  %s\n", "BOLT-ITEM-CODE", bolt_charx_list[index].value.s_value );
                 strcpy ( bolt_item_code, bolt_charx_list[index].value.s_value );
             }
             
         }
         /* If we didnt find an item code, create one */
         if ( !strcmp ( bolt_item_code, "" ) )
         {
             sprintf ( bolt_item_code, "%s-%u", "bolt", udo_tag );
             sprintf ( pcf_string, "    %s  %s\n", "BOLT-ITEM-CODE", bolt_item_code );
         }
         write_string ( pcf_string, pcf_stream );
         strcpy ( pcf_string, "" );
         
         /* Check for unique item code */
         num_items = *n_items;
         for ( ii = 0;  ii < num_items; ii++ )
         {
             if ( !strcmp ( (*item_code)[ii], bolt_item_code ) )
             {
                 item_code_is_unique = FALSE;
                 break;
             }
         }
         if ( item_code_is_unique )
         {
             *item_code = (char **) UF_reallocate_memory( *item_code, 
                                                          sizeof (char *)*(num_items +1),
                                                          &err_code );
             
             (*item_code)[num_items] = (char * )UF_allocate_memory( strlen( bolt_item_code ) + 1,
                                                                    &err_code );
             strcpy( (*item_code)[num_items], bolt_item_code );
             num_items += 1;
         }
         *n_items = num_items;

         /* Write the materials stream */
         if ( item_code_is_unique )
         {
             sprintf ( pcf_string, MAT_ITEM_CODE_FMT, bolt_item_code );
             write_string ( pcf_string, material_stream );

             UF_ROUTE_find_title_in_charx ( bolt_charx_count, bolt_charx_list, "MATERIAL", &index );

             if (index > -1) 
             {
                 sprintf( pcf_string, MAT_MATERIAL_FMT, bolt_charx_list[index].value.s_value ); 
                 status = write_string( pcf_string, material_stream );
             }
         }
         if ( is_bolt )
         { 
             if( charx_count > 0 )
             {
                 status = UF_ROUTE_free_charx_array( bolt_charx_count, bolt_charx_list );
                 CHECK_STATUS

                 charx_count = 0;
                 charx_list = NULL;
             }
             break;
         }

   }while ( udo_tag !=NULL_TAG );
}
/*********************************************************************
 * Function Name: apply_component_attribute
 * Function_Description: Look for the input attribute on component/stock 
 *     and if found apply the attribute to the component/stock
 * Input
 *     counter  - attribute counter
 *     obj_tag  - tag of component or stock
 *     char* title - attribute title
 * Output
 * Returns
 */
void pcf_generation::apply_component_attribute ( int counter, tag_t obj_tag, char* title, FILE * pcf_stream )
{
      int found = 0;
      char attr_value[ MAX_LINE_BUFSIZE ] ="";
      char pcf_string[ MAX_LINE_BUFSIZE ] ="";

      tag_t lookup_tag = obj_tag;

      //Check if the input object is stock, and get the stock data for the stock
      tag_t stock_data = NULL_TAG;
      UF_ROUTE_ask_stock_stock_data( obj_tag, &stock_data );

      //Find attribute on stock or component
      UF_ATTR_find_attribute ( lookup_tag, UF_ATTR_any, title, &found );

      if ( !found && stock_data != NULL_TAG )
      {
          UF_ATTR_find_attribute ( stock_data, UF_ATTR_any, title, &found );

          if ( found )
              lookup_tag = stock_data;
      }

      if ( found )
      {
          UF_ATTR_value_t value;
          UF_ATTR_read_value ( lookup_tag, title, UF_ATTR_any, &value );

          switch (found)
          {
          case UF_ATTR_integer:
              sprintf ( attr_value, "%d", value.value.integer ); 
              sprintf ( pcf_string, "    %s%d  %s\n", "COMPONENT-ATTRIBUTE", counter, attr_value );
              write_string ( pcf_string, pcf_stream );
              break;
          case UF_ATTR_real:
              sprintf ( attr_value, "%.4f", value.value.real ); 
              sprintf ( pcf_string, "    %s%d  %s\n", "COMPONENT-ATTRIBUTE", counter,attr_value );
              write_string ( pcf_string, pcf_stream );
              break;
          case UF_ATTR_string:
              sprintf ( attr_value, "%s", value.value.string ); 
              sprintf ( pcf_string, "    %s%d  %s\n", "COMPONENT-ATTRIBUTE", counter, attr_value );
              write_string ( pcf_string, pcf_stream );
              break;
          default:
              break;
          }
      }
      else //write an empty COMPONENT-ATTRIBUTExx
      {
          sprintf ( pcf_string, "    %s%d\n", "COMPONENT-ATTRIBUTE", counter );
          write_string ( pcf_string, pcf_stream );
      }
}

/**********************************************************************
* Function Name: build_components
* 
* Function Description: Build the components items for the PCF.  Items 
*     include all of the UG/Piping stock and components along with 
*     their required attributes (routing characteristics).
*
* Input: tag_t part_tag - tag of part used to create PCF Data
*        FILE * pcf_stream - FILE stream of PCF 
*        FILE * material_stream - FILE stream of temporary material file
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::build_components( tag_t part_tag,
                             FILE * pcf_stream,
                             FILE * material_stream )
{
   tag_t stock_data = NULL_TAG;
   tag_t comp_tag = NULL_TAG;
   tag_t *segments = NULL;
   double end_pt1[3];
   double end_pt2[3];
   UF_ROUTE_charx_p_t charx_list;
   int status = ERROR_OK;
   int index = 0;
   int num_segments = 0;
   int charx_count;
   int num_rcps = 0;
   tag_t *rcps = NULL;
   
   int n_items = 0;

   char **item_code_char = NULL;
   char tso_pcf_string[ MAX_FSPEC_BUFSIZE ] = "";

   
   /*
   ** Get all of the stock and build/write a PCF entry for each valid stock
   ** segment.
   */

   //*****add by CJ********Find the wcs************
	double end_pt1_abs[3];
    double end_pt2_abs[3];
	std::vector<double> label_coordinate[4];
	double direction[3];

	int pipe_count = 0;
	int bend_count = 0;

	//******Find the point pointed by the Welding Lable********//
	Annotations::LabelCollection *label_collection = workPart->Labels();
	Annotations::LabelCollection::iterator it_label;
	for(it_label = label_collection->begin() ; it_label != label_collection->end() ; it_label ++ )	
	{
		Annotations::Label* label = (Annotations::Label*) *it_label;
		std::vector< NXString > title = label->GetText();

		char *title_str = new char[strlen(title[0].GetLocaleText())+1];
		strcpy(title_str, title[0].GetLocaleText());

		if(strstr(title_str, "TACK WELD") != NULL || strstr(title_str, "EXTRA LENGTH") != NULL || strstr(title_str, "NO WELD") != NULL)
		{
			Annotations::LeaderBuilder *leader_builder =  workPart->Annotations()->CreateDraftingNoteBuilder(label)->Leader();
			Annotations::LeaderDataList *leader_data_list = leader_builder->Leaders();

			for(int i_label = 0; i_label < leader_data_list->Length(); i_label++)
			{
				Annotations::LeaderData *leader_data = leader_data_list->FindItem(i_label);
				DisplayableObject *selection;
				View *view;
				Point3d point;
				leader_data->Leader()->GetValue(&selection, &view, &point);

				label_coordinate[0].push_back(point.X);
				label_coordinate[1].push_back(point.Y);
				label_coordinate[2].push_back(point.Z);
				label_coordinate[3].push_back(0.0);
			}
		}

		if(strstr(title_str, "CUT ELBOW") != NULL)
		{
			Annotations::LeaderBuilder *leader_builder =  workPart->Annotations()->CreateDraftingNoteBuilder(label)->Leader();
			Annotations::LeaderDataList *leader_data_list = leader_builder->Leaders();

			for(int i_label = 0; i_label < leader_data_list->Length(); i_label++)
			{
				Annotations::LeaderData *leader_data = leader_data_list->FindItem(i_label);
				DisplayableObject *selection;
				View *view;
				Point3d point;
				leader_data->Leader()->GetValue(&selection, &view, &point);

				double point_abs[3];
				double point_r[3];
				point_abs[0] = point.X;
				point_abs[1] = point.Y;
				point_abs[2] = point.Z;
				coordinate_transform(point_abs,point_r);

				pointinfo point_temp;
				point_temp.end_point[0] = point_r[0];
				point_temp.end_point[1] = point_r[1];
				point_temp.end_point[2] = point_r[2];
				point_temp.diameter = 0.0;
				cut_elbow_label.push_back(point_temp);
				//uc1601("test",1);
			}
		}
	}

	NXOpen::Routing::StockCollection::iterator it_stock;
	for(it_stock = workPart->RouteManager()->Stocks()->begin() ; it_stock != workPart->RouteManager()->Stocks()->end() ; it_stock ++ )				
	{
		bool has_spool_identifier = false;
		char spool_identifier[MAX_LINE_BUFSIZE];
		char pcf_string[MAX_LINE_BUFSIZE];

		Routing::Stock* stocks = (Routing::Stock*) *it_stock;
		//get feature from stock
		std::vector< NXOpen::Features::Feature * > feature = stocks->GetFeatures();		

		if(feature.size()>0)
		{
			//get value of "TubeCallout" from feature
						 
			if(feature[0]->HasUserAttribute("KIT",NXOpen::NXObject::AttributeTypeString,0))
			{
				strcpy(spool_identifier, feature[0]->GetStringUserAttribute("KIT",0).GetLocaleText());
				has_spool_identifier = true;
			}else
			{
				has_spool_identifier = false;
			}
		}else
		{
			uc1601("Can't find the feature from stock! PCF generation uncomplete!",1);
		}

		 /*
         ** Get all of the charxs for the stock and find the charx required for
         ** the stock entry in the PCF file      
         */ 
		status = UF_ROUTE_ask_stock_stock_data( stocks->Tag(), &stock_data);
        CHECK_STATUS
        status = UF_ROUTE_ask_characteristics ( stock_data, UF_ROUTE_CHARX_TYPE_ANY, &charx_count, &charx_list );
        CHECK_STATUS
        status = UF_ROUTE_ask_stock_segments( stocks->Tag(), &num_segments, &segments );
        CHECK_STATUS

         for ( int jj = 0; jj < num_segments; jj++ )
         {
             logical is_corner = FALSE;
             int cnr_type      = 0;
             tag_t cnr_rcp     = NULL_TAG;
             tag_t cnr_obj     = NULL_TAG;
             tag_t end_rcps[2] = {NULL_TAG, NULL_TAG};

             status = UF_ROUTE_ask_segment_end_pnts ( segments[jj],
                                                      end_pt1_abs,
                                                      end_pt2_abs );		

			 //*****add by CJ********Transfer the coordinate to wcs************
			 coordinate_transform(end_pt1_abs, end_pt1);
			 coordinate_transform(end_pt2_abs, end_pt2);

             CHECK_STATUS
             
             status = UF_ROUTE_ask_seg_rcps ( segments[jj], end_rcps );

             CHECK_STATUS
             
             /* need to adjust the end points for TEE SET ON condition*/
             adjust_end_pts_for_tee_set_on ( end_rcps, end_pt1, end_pt2 );

             /* Check for corner segment, it needs to be treated as bend
                and not pipe, see pcf documentation */
             is_corner = UF_ROUTE_ask_obj_corner_info ( segments[jj],
                                                        &cnr_type,
                                                        &cnr_rcp,
                                                        &cnr_obj );
             /*
             ** Write the component id for the stock
             */
             if ( is_corner  && cnr_type == UF_ROUTE_CORNER_BEND )
             {
                 char string[ MAX_LINE_BUFSIZE ] = "";
                 
                 strcpy ( string, BEND_PIPE_ID );
                 strcat ( string, "\n" );
                 status = write_string ( string, pcf_stream );
             }
             else
             {
                 write_component_id( charx_count, charx_list, pcf_stream );
             }

             /* determine in and out diameters, then write the end points and diameters for the stock */
             double inDiameter  = determine_in_diameter( stock_data );
             double outDiameter = determine_out_diameter( stock_data, inDiameter );
             write_end_points( end_pt1, end_pt2, inDiameter, outDiameter, pcf_stream ); 

			pointinfo point_temp;
			point_temp.end_point[0] = end_pt1[0];
			point_temp.end_point[1] = end_pt1[1];
			point_temp.end_point[2] = end_pt1[2];
			point_temp.diameter = inDiameter;
			pipe_endpoint.push_back(point_temp);
			point_temp.end_point[0] = end_pt2[0];
			point_temp.end_point[1] = end_pt2[1];
			point_temp.end_point[2] = end_pt2[2];
			point_temp.diameter = outDiameter;
			pipe_endpoint.push_back(point_temp);

			  //*******判断端点是否为weld点,是weld点，将此直径赋予该weld***************//
			 for (int count = 0; count < label_coordinate[0].size(); count++)
			 {
				 double label_point[3]={label_coordinate[0][count],label_coordinate[1][count],label_coordinate[2][count]};

				 if(check_same_point(end_pt1_abs, label_point))
				 {
					 label_coordinate[3][count] = inDiameter;
				 }else if(check_same_point(end_pt2_abs, label_point))
				 {
					 label_coordinate[3][count] = inDiameter;
				 }
			 }
             
             /* For bend corner, write center point, which is same as corner rcp
                    position and the SKEY */
             if ( is_corner && cnr_type == UF_ROUTE_CORNER_BEND )
             {
				 bend_count++;

                 int err_code = 0;
                 double center_pnt_abs[3];
				 double center_pnt[3];
				 double bend_radius = 0.0;
                 char *pcf_string = NULL;

                 UF_ROUTE_ask_rcp_position ( cnr_rcp, center_pnt_abs);
				 coordinate_transform(center_pnt_abs, center_pnt);

                 pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                                           &err_code );

                 sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                           center_pnt[0], center_pnt[1], center_pnt[2]);

                 status = write_string( pcf_string, pcf_stream );
                 UF_free ( pcf_string );

				 //write bend radius
				 bend_radius = line_length(center_pnt, end_pt1);
				 pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE, &err_code );
				 sprintf ( pcf_string, "    BEND-RADIUS %.4f\n", bend_radius);
				 status = write_string( pcf_string, pcf_stream );
                 UF_free ( pcf_string );

                 /* Bend needs skey. We assume plain end connection, see pcf documentation
                    for furter information */
                 pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,  &err_code );
                 sprintf ( pcf_string, COMP_SKEY_FMT, BEND_PIPE_SKEY );
                 status = write_string( pcf_string, pcf_stream );
                 UF_free ( pcf_string );

             }

             /*
             ** Write the item code for the stock and include any unique items
             ** in the material list.
             */
			 write_item_code( charx_count, charx_list, stocks->Tag(), pcf_stream,
                              material_stream, &item_code_char, &n_items );

             /*
             ** Write the weight for the stock  
             */
             write_weight( charx_count, charx_list, pcf_stream );

			 if(has_spool_identifier)
			 {
				 sprintf(pcf_string,"    SPOOL-IDENTIFIER  %s\n", spool_identifier);
				 write_string(pcf_string, pcf_stream);
			 }else
			 {
				 uc1601("The stock doesn't have KIT attribute!",1);
			 }

			 /*
			 ** Free up the routing characteristics and prepare it for use again
			 */
			 if( charx_count > 0 )
			 {
				status = UF_ROUTE_free_charx_array( charx_count,
													charx_list );
				CHECK_STATUS

				charx_count = 0;
				charx_list = NULL;
			 }
         }
				
	}


   /*
   ** Get all of the components and build/write a PCF entry for each valid component.
   ** Components that are not defined for ISOGEN are assigned a MISC_COMP components.
   */

   //find stubend endpoint at beginning
	for(int it_comp = 0; it_comp < components.size(); it_comp++)
	{
		if( strcmp(components[it_comp]->GetStringAttribute("ISOGEN_COMPONENT_ID").GetLocaleText(), "LAPJOINT-STUBEND" ) == 0 
			|| strcmp(components[it_comp]->GetStringAttribute("ISOGEN_COMPONENT_ID").GetLocaleText(), "LAPJOINT-STUB-END" ) == 0 )
		{
			get_stub_end_point(components[it_comp]->Tag());
		}
	}

	for(int it_comp = 0; it_comp < components.size(); it_comp++)
	{

      tag_t stock_tag = NULL_TAG;

	  comp_tag = components[it_comp]->Tag();
      UF_ROUTE_ask_object_stock ( comp_tag, &stock_tag );

      if ( stock_tag != NULL_TAG )
          continue;

      CHECK_STATUS

      /*
      ** Get all of the charx for the component
      */
      status = UF_ROUTE_ask_characteristics( comp_tag, 
                                             UF_ROUTE_CHARX_TYPE_ANY,
                                             &charx_count, 
                                             &charx_list);
      CHECK_STATUS

      if (status != ERROR_OK || charx_count == 0 )
      {
         char syslog_message[133]="";

         sprintf( syslog_message, "Failed to find any characteristics for component: %u \n", comp_tag );
         UF_print_syslog( syslog_message,    
                          FALSE );
         /* 
         ** Go ahead an continue cycling the components, In an assembly where
         ** there is a subassembly, the subassembly will not have any charx's
         */
         continue;
      }

      /* check for valid charx: component should have part number 
      ** and material and NPS charx defined 
      */
      status = has_part_no_and_material ( comp_tag, charx_count, charx_list );

      if ( status != ERROR_OK )
      {
          /* ignore this component, we wont be able to provide 
          ** the mandatory information to pcf anyways
          */
          continue;
      }
         
      status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                             ISO_CHARX_COMP_ID_STR, 
                                             &index);

      /*  
      ** Write the common items for each component.  Includes the component ID,
      ** end_points, skey, item code, and weight.  If the item is not found
      ** for the component (ex. not all components have a skey), the item is 
      ** not written.
      */
      write_component_id( charx_count, charx_list, pcf_stream );

      if (status != ERROR_OK)
	  {
		  uc1601("The compent has no ISOGEN_COMPONENT_ID!",1);
		  continue;
	  }
	  /* Outlet components need center point and branch point */
      else if( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          strcmp( charx_list[index].value.s_value, "OLET" ) == 0 )
      {
          write_olet_center_and_branch_points ( comp_tag, pcf_stream );
      }
      else if ( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          strcmp( charx_list[index].value.s_value, "SUPPORT" ) == 0 )
      {
          //Support needs co-ords and not end points
          write_support_coords( part_tag, comp_tag, pcf_stream );
      }
	  //add  by CJ
	  else if ( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          (strcmp( charx_list[index].value.s_value, "LAPJOINT-STUBEND" ) == 0 || strcmp( charx_list[index].value.s_value, "LAPJOINT-STUB-END" ) == 0))
      {
          //LAPJOINT-STUB-END
          write_stub_end_point(comp_tag, pcf_stream );
      }
	  else if ( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          strcmp( charx_list[index].value.s_value, "FLANGE-BLIND" ) == 0 )
      {
          //Flange Blind
          write_flange_blind_point(part_tag, comp_tag, pcf_stream );
      }
	  else if ( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          strcmp( charx_list[index].value.s_value, "FLANGE" ) == 0 )
      {
          //Flange Blind
          write_flange_point(part_tag, comp_tag, pcf_stream );
      }
	  else if ( !status &&
          index >= 0 &&
          charx_list[index].value.s_value != NULL &&
          strcmp( charx_list[index].value.s_value, "ELBOW" ) == 0 )
      {
          //ELBOW
          write_elbow_point(part_tag, comp_tag, pcf_stream );
      }
      else
      {
          write_end_and_branch_points( part_tag, comp_tag, pcf_stream );
      }
      
      
      /* 
      ** If the Component ID is not found, the component has already been assigned
      ** as a MISC-COMP (in write_component_id).  Write the center point if the 
      ** component has one. 
      */
      if (status != ERROR_OK)  
      {
         //write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream ); 
         status = ERROR_OK;
      }
      /*
      ** Write the items specific to the component (ex. angle for ELBOWS,
      ** center point for TEES).  Note that components requiring
      ** branch points have already had them written.  
      */
      else 
      {
          int skey_index = 0;

          status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                             ISO_CHARX_SKEY_STR, 
                                             &skey_index);

         /* 
         ** Cross components require a center point
         */
         if( strcmp( charx_list[index].value.s_value, CROSS_COMP_ID ) == 0 )
         {
            write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );
         }
            
        
         /*
         ** Instrument components require a center point
         */
         if( strcmp(charx_list[index].value.s_value, INSTRUMENT_COMP_ID ) == 0 )
         {
            write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );
         }
            
         /*
         ** Tee components require a center point
         */
         if( strcmp( charx_list[index].value.s_value, TEE_COMP_ID ) == 0 )
         {
            write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );
         }
            
         /*
         ** Valve components require a center point
         */
         if( strcmp( charx_list[index].value.s_value, VALVE_COMP_ID ) == 0 ||
             strcmp( charx_list[index].value.s_value, "VALVE-ANGLE" ) == 0 )
         {
            write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );
         }

         /* latrolets need angle data */
         
         if ( !status &&
              charx_list[skey_index].value.s_value != NULL &&
              ( !strcmp ( charx_list[skey_index].value.s_value, "LABW" ) ||
                !strcmp ( charx_list[skey_index].value.s_value, "LASC" ) ||
                !strcmp ( charx_list[skey_index].value.s_value, "LASW" )   ) )
         {
             char   angle_string[ MAX_LINE_BUFSIZE ] = "";
             int    angle_index = 0;
             int    angle_status = 0;
             double def_angle =  45.00;

             angle_status = UF_ROUTE_find_title_in_charx ( charx_count, charx_list, 
                                                           "LATERAL_ANGLE",
                                                           &angle_index );

             if ( angle_status == ERROR_OK )
                 sprintf ( angle_string, COMP_ANGLE_FMT, 
                           charx_list[angle_index].value.r_value );
             else 
                 sprintf ( angle_string, COMP_ANGLE_FMT, def_angle );

             write_string ( angle_string, pcf_stream );

             strcpy ( angle_string, "" );
         }
         status = ERROR_OK;
      }
      /* Add BOLT/STUD and NUTS info to GASKETs
       */
      if( index > -1 && strcmp( charx_list[index].value.s_value, "GASKET" ) == 0 )
      {
          write_bolts_info( part_tag, comp_tag, charx_count, charx_list, 
                            pcf_stream, material_stream, &item_code_char, &n_items );
      }

      write_skey( charx_count, charx_list, pcf_stream ); 
      
      write_item_code( charx_count, charx_list, comp_tag, pcf_stream,
                       material_stream, &item_code_char, &n_items );
      
      write_weight( charx_count, charx_list, pcf_stream);

	  write_comp_spool_identifier(comp_tag, pcf_stream );

      /* Add COMPONENT-ATTRIBUTE1 - 9 attributes */
      {
          for ( int kk = 0; kk < n_pcf_comp_attrs; kk++ )
          {
              apply_component_attribute( kk, comp_tag, pcf_comp_attrs[kk], pcf_stream ); 
          }
      }

      /*
      ** Free up the routing characteristics and prepare it for use again
      */
      if( charx_count > 0 )
      {
         status = UF_ROUTE_free_charx_array( charx_count,
                                             charx_list );
         CHECK_STATUS
  
         charx_count = 0;
         charx_list = NULL;
      }

   }

   //find gaps which will add welding
   for(int it_point = 0; it_point < control_point.size(); it_point ++)
   {
	   for(int j = it_point; j < control_point.size(); j ++)
	   {
		   double gap = line_length(control_point[it_point].end_point, control_point[j].end_point);
		   if(gap > 0.001 && gap < 0.1)
		   {
			   weldinfo temp;
			   temp.point_1[0] = control_point[it_point].end_point[0];
			   temp.point_1[1] = control_point[it_point].end_point[1];
			   temp.point_1[2] = control_point[it_point].end_point[2];
			   temp.diameter_1 = control_point[it_point].diameter;
			   temp.point_2[0] = control_point[j].end_point[0];
			   temp.point_2[1] = control_point[j].end_point[1];
			   temp.point_2[2] = control_point[j].end_point[2];
			   temp.diameter_2 = control_point[j].diameter;
			   additional_weld.push_back(temp);
		   }
	   }
   }

   //******find the weldings marked with labels********
	Annotations::LabelCollection::iterator it_label_2;
	for(it_label_2 = label_collection->begin() ; it_label_2 != label_collection->end() ; it_label_2 ++ )	
	{
		double weld_endpoint[3];
		double weld_endpoint_r[3];
		double diameter = 0.0;
		char skey_string[100];
		char category_string[100];
		
	
		Annotations::Label* label = (Annotations::Label*) *it_label_2;
		std::vector< NXString > title = label->GetText();
		Annotations::LeaderBuilder *leader_builder =  workPart->Annotations()->CreateDraftingNoteBuilder(label)->Leader();
		Annotations::LeaderDataList *leader_data_list = leader_builder->Leaders();
		for(int i_label = 0; i_label < leader_data_list->Length(); i_label++)
		{
			Annotations::LeaderData *leader_data = leader_data_list->FindItem(i_label);
			DisplayableObject *selection;
			View *view;
			Point3d point;
			leader_data->Leader()->GetValue(&selection, &view, &point);

			weld_endpoint[0] = point.X;
			weld_endpoint[1] = point.Y;
			weld_endpoint[2] = point.Z;

			coordinate_transform(weld_endpoint, weld_endpoint_r);
			for (int count = 0; count < label_coordinate[0].size(); count++)
			{
				double label_point[3]={label_coordinate[0][count],label_coordinate[1][count],label_coordinate[2][count]};
				if(check_same_point(weld_endpoint, label_point))
				{
					diameter = label_coordinate[3][count];
					break;
				}
			}

			if(diameter > 0.0001)
			{

				bool tack_weld = false;
				bool extra_length = false;
				bool no_weld = false;
				for(int k = 0; k < title.size() ; k++)
				{
					char *title_str = new char[strlen(title[k].GetLocaleText())+1];
					strcpy(title_str, title[k].GetLocaleText());
					if(strstr(title_str,"TACK WELD")!= NULL)
					{
						tack_weld = true;
					}
					if(strstr(title_str,"EXTRA LENGTH")!=NULL)
					{
						extra_length = true;
					}
					if(strstr(title_str,"NO WELD")!= NULL)
					{
						no_weld = true;
					}
				}
		
				if(!tack_weld && extra_length)
				{
					bool overlap = false;
					vector<weldinfo>::iterator overlap_count;
					for(vector<weldinfo>::iterator it = additional_weld.begin();it!=additional_weld.end();it++)
					{
						weldinfo temp_weld = *it;
						if(check_same_point(weld_endpoint_r,temp_weld.point_1) || check_same_point(weld_endpoint_r,temp_weld.point_2))
						{
							overlap = true;
							overlap_count = it;
							break;
						}
					}
					if(overlap)
					{
						weldinfo overlap_weld = *overlap_count;
						write_string("WELD\n",pcf_stream);
						write_end_points( overlap_weld.point_1, overlap_weld.point_2, overlap_weld.diameter_1, overlap_weld.diameter_2, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WF"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
						additional_weld.erase(overlap_count);
					}else
					{
						write_string("WELD\n",pcf_stream);
						write_end_points( weld_endpoint_r, weld_endpoint_r, diameter, diameter, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WF"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
					}
				}

				if(tack_weld && extra_length){
					bool overlap = false;
					vector<weldinfo>::iterator overlap_count;
					for(vector<weldinfo>::iterator it = additional_weld.begin();it!=additional_weld.end();it++)
					{
						weldinfo temp_weld = *it;
						if(check_same_point(weld_endpoint_r,temp_weld.point_1) || check_same_point(weld_endpoint_r,temp_weld.point_2))
						{
							overlap = true;
							overlap_count = it;
							break;
						}
					}
					if(overlap)
					{
						weldinfo overlap_weld = *overlap_count;
						write_string("WELD\n",pcf_stream);
						write_end_points( overlap_weld.point_1, overlap_weld.point_2, overlap_weld.diameter_1, overlap_weld.diameter_2, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WFT"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
						additional_weld.erase(overlap_count);
					}else
					{
						write_string("WELD\n",pcf_stream);
						write_end_points( weld_endpoint_r, weld_endpoint_r, diameter, diameter, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WFT"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
					}
				}
			
				if(tack_weld && !extra_length){
					bool overlap = false;
					vector<weldinfo>::iterator overlap_count;
					for(vector<weldinfo>::iterator it = additional_weld.begin();it!=additional_weld.end();it++)
					{
						weldinfo temp_weld = *it;
						if(check_same_point(weld_endpoint_r,temp_weld.point_1) || check_same_point(weld_endpoint_r,temp_weld.point_2))
						{
							overlap = true;
							overlap_count = it;
							break;
						}
					}
					if(overlap)
					{
						weldinfo overlap_weld = *overlap_count;
						write_string("WELD\n",pcf_stream);
						write_end_points( overlap_weld.point_1, overlap_weld.point_2, overlap_weld.diameter_1, overlap_weld.diameter_2, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WST"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
						additional_weld.erase(overlap_count);
					}else
					{
						write_string("WELD\n",pcf_stream);
						write_end_points( weld_endpoint_r, weld_endpoint_r, diameter, diameter, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WST"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "ERECTION"); 
						write_string( category_string, pcf_stream );
					}
				}

				if(no_weld){
					bool overlap = false;
					vector<weldinfo>::iterator overlap_count;
					for(vector<weldinfo>::iterator it = additional_weld.begin();it!=additional_weld.end();it++)
					{
						weldinfo temp_weld = *it;
						if(check_same_point(weld_endpoint_r,temp_weld.point_1) || check_same_point(weld_endpoint_r,temp_weld.point_2))
						{
							overlap = true;
							overlap_count = it;
							break;
						}
					}
					if(overlap)
					{
						weldinfo overlap_weld = *overlap_count;
						write_string("WELD\n",pcf_stream);
						write_end_points( overlap_weld.point_1, overlap_weld.point_2, overlap_weld.diameter_1, overlap_weld.diameter_2, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WO"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "OFFSHORE"); 
						write_string( category_string, pcf_stream );
						additional_weld.erase(overlap_count);
					}else
					{
						write_string("WELD\n",pcf_stream);
						write_end_points( weld_endpoint_r, weld_endpoint_r, diameter, diameter, pcf_stream ); 
						sprintf( skey_string, COMP_SKEY_FMT, "WO"); 
						write_string( skey_string, pcf_stream );
						sprintf( category_string, "    CATEGORY %s\n", "OFFSHORE"); 
						write_string( category_string, pcf_stream );
					}
				}
			}

		}		
	}
	
	//write fabrication weld
	for(int i = 0; i < additional_weld.size(); i++)
	{
		char skey_string[100];
		char category_string[100];
		
		write_string("WELD\n",pcf_stream);
		write_end_points( additional_weld[i].point_1, additional_weld[i].point_2, additional_weld[i].diameter_1, additional_weld[i].diameter_2, pcf_stream ); 
		sprintf( skey_string, COMP_SKEY_FMT, "WW"); 
		write_string( skey_string, pcf_stream );
		sprintf( category_string, "    CATEGORY %s\n", "FABRICATION"); 
		write_string( category_string, pcf_stream );
	}

    /* Cycle through all rcps, look for TEE like condition, i.e. three segments 
    * meeting at RCP forming a 'T', check if the rcp has TEE_SET_ON attribute
    * defined and print all the isogen information for TEE_SET_ON component 
    */

   // status = get_tee_set_on_components ( part_tag, &num_rcps, &rcps );

   //for ( int i = 0; i < num_rcps; i++ )
   //{
   //    double tso_center[3] = {0.0, 0.0, 0.0};
   //    double tso_branch[3] = {0.0, 0.0, 0.0};
	  // double tso_center_r[3] = {0.0, 0.0, 0.0};
   //    double tso_branch_r[3] = {0.0, 0.0, 0.0};

   //   status = UF_ROUTE_ask_characteristics( rcps[i], 
   //                                          UF_ROUTE_CHARX_TYPE_ANY,
   //                                          &charx_count, 
   //                                          &charx_list);

   //   CHECK_STATUS

   //   if (status != ERROR_OK || charx_count == 0 )
   //   {
   //      char syslog_message[133]="";

   //      sprintf( syslog_message, "Failed to find any characteristics rcp: %u \n", rcps[i] );
   //      UF_print_syslog( syslog_message,    
   //                       FALSE );
   //      /* 
   //      ** Go ahead an continue cycling the components, In an assembly where
   //      ** there is a subassembly, the subassembly will not have any charx's
   //      */
   //      continue;
   //   }

   //   write_component_id( charx_count, charx_list, pcf_stream );
   //   
   //   get_center_and_branch_points_for_tee_set_on( rcps[i], charx_count, 
   //                                                charx_list, 
   //                                                tso_center, tso_branch);

	  ////Add by CJ
	  //coordinate_transform(tso_branch,tso_branch_r);
	  //coordinate_transform(tso_center,tso_center_r);

   //   /*sprintf( tso_pcf_string, COMP_BRANCH1_POINT_FMT, 
   //       tso_branch[0], tso_branch[1], tso_branch[2], 
   //       charx_list[index].value.r_value );*/
	  //sprintf( tso_pcf_string, COMP_BRANCH1_POINT_FMT, 
   //       tso_branch_r[0], tso_branch_r[1], tso_branch_r[2], 
   //       charx_list[index].value.r_value );

   //   write_string( tso_pcf_string, pcf_stream );
   //   strcpy ( tso_pcf_string, "" );

   //   /*sprintf( tso_pcf_string, COMP_CENTER_POINT_FMT, 
   //       tso_center[0], tso_center[1], tso_center[2]);*/
	  //sprintf( tso_pcf_string, COMP_CENTER_POINT_FMT, 
   //       tso_center_r[0], tso_center_r[1], tso_center_r[2]);
   //   write_string ( tso_pcf_string, pcf_stream );
   //   strcpy ( tso_pcf_string, "" );

   //   write_skey( charx_count, charx_list, pcf_stream ); 
   //   
   //   write_item_code( charx_count, charx_list, rcps[i], pcf_stream,
   //                    material_stream, &item_code_char, &n_items );
   //   
   //   write_weight( charx_count, charx_list, pcf_stream); 
   //   if( charx_count > 0 )
   //   {
   //      status = UF_ROUTE_free_charx_array( charx_count,
   //                                          charx_list );
   //      CHECK_STATUS
  
   //      charx_count = 0;
   //      charx_list = NULL;
   //   }
   //}

   if ( num_rcps )
      UF_free ( rcps );
   
   if ( item_code_char != NULL )
   {
      for ( int i = 0; i < n_items; i++ )
         UF_free ( item_code_char[i] );

      UF_free ( item_code_char );
   }
   
   return( status );
}

/**********************************************************************
* Function Name: write_component_id
* 
* Function Description: Write the ISOGEN Component ID for the given 
*     component characteristics.  If an ISOGEN Component ID, assign
*     MISC-COMP as the ID. 
*
* Input: int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * pcf_stream - FILE stream of the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_component_id( int charx_count,
                               UF_ROUTE_charx_p_t charx_list,
                               FILE * pcf_stream )

{
   int status = ERROR_OK;
   int index = 0; 
   int index_1 = 0;
   char string[  MAX_LINE_BUFSIZE ] = "";
   bool is_pipe = false;
   bool is_tube = false;

    status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, UF_ROUTE_DESCRIPTION_STR, &index_1);
	if ( status == ERROR_OK && index_1 >= 0) 
	{
		char *pipe = strstr(charx_list[index_1].value.s_value,"PIPE");
		char *tube = strstr(charx_list[index_1].value.s_value,"TUBE");
		
		if(pipe!=NULL){
			is_pipe = true;
		}
		if(tube!=NULL){
			is_tube = true;
		}
	}
   /*
   ** Find the Component ID in the routing charx.  If not found, make
   ** the ID MISC-COMPONENT
   */
   status = UF_ROUTE_find_title_in_charx( charx_count,
                                          charx_list,
                                          ISO_CHARX_COMP_ID_STR,
                                          &index );
   if (status != ERROR_OK)
   {
	   if(is_pipe){
		   strcpy( string, PIPE_ID );
		   strcat( string, "\n" );
		   status = write_string( string, pcf_stream );
	   }else if(is_tube){
		   strcpy( string, PIPE_ID );
		   strcat( string, "\n" );
		   status = write_string( string, pcf_stream );
	   }else{
		   //strcpy( string, MISC_COMP_ID );
		   //strcat( string, "\n" );
		   //status = write_string( string, pcf_stream );
	   }
   }
   else
   {
      strcpy( string, (char *)charx_list[index].value.s_value );
      strcat( string, "\n" );
      status = write_string( string,
                             pcf_stream);
   }

    return( status );
}

/***********************************************************************
* Function Name: find_nps_branch_index
* 
* Function Description: find the value of NPS_BRANCH either by that name or by its aliases
*        Note: UF_ROUTE_find_title_in_charx returns index = -1 if the title is not found
*
* Input:
*        int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
* Output:
*        int* index - index into charx array where the matching attribute is found
* 
* Returns: ERROR_OK or UF_err_operation_aborted
**************************************************************************/
int pcf_generation::find_nps_branch_index ( int charx_count, 
                                 UF_ROUTE_charx_p_t charx_list, 
                                 int* index )
{
       int status = ERROR_OK;

       /* look for NPS_OUT in charx */
       status = UF_ROUTE_find_title_in_charx( charx_count, 
                                              charx_list, 
                                              UF_ROUTE_NPS_OUT_STR, 
                                              index);

       /* if not NPS_OUT not found, look for NPS_BRANCH */
       if ( *index < 0 )
       {
            status = UF_ROUTE_find_title_in_charx( charx_count, 
                                                   charx_list, 
                                                   UF_ROUTE_NPS_BRANCH_STR, 
                                                   index);
       }

       return status;
}
/**********************************************************************
* Function Name: write_end_points
* 
* Function Description: Write the end point attributes for the given 
*     component/stock characteristics.
*
* Input:
*        double end_pt1[3]  - end point
*        double end_pt2[3]  - end point
*        double inDiameter  - in diameter of component  ( NPS )
*        double outDiameter - out diameter of component ( NPS_OUT, NPS_BRANCH )
*        FILE * pcf_stream  - FILE stream of the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_end_points( double end_pt1[3],
                             double end_pt2[3],
                             double inDiameter,
                             double outDiameter,
                             FILE * pcf_stream )

{
   int status = ERROR_OK;
   char *pcf_string;
   int err_code = ERROR_OK;
   
   /*
   ** Allocate the string to be built/written
   */
   pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                             &err_code );
   
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation for end point string.\n", FALSE );
      return( UF_err_operation_aborted );
   }
   
   /*
   ** Build and write each end point line
   */
   sprintf( pcf_string, COMP_END_POINT_FMT, end_pt1[0], end_pt1[1], end_pt1[2], inDiameter );  
   status = write_string( pcf_string, pcf_stream );
   
   sprintf( pcf_string, COMP_END_POINT_FMT, end_pt2[0], end_pt2[1], end_pt2[2], outDiameter);  
   status = write_string( pcf_string, pcf_stream );
   
   UF_free( pcf_string );
   
   return (status);
}

/**********************************************************************
* Function Name: write_item_code
* 
* Function Description: If the item code is unique, write the item 
*     code for the given component characteristics.
*
* Input: int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * pcf_stream - FILE stream of PCF
*        FILE * material_stream - FILE stream oftemporary Material file
* 
* Returns:
*     ERROR_OK if OK
*     error code (ERROR_raise) if error 
*
***********************************************************************/
int pcf_generation::write_item_code( int charx_count,
                            UF_ROUTE_charx_p_t charx_list,
                            tag_t comp_tag,
                            FILE * pcf_stream,
                            FILE * material_stream,
                            char *** item_code_char,
                            int *n_items )
{
   int status = ERROR_OK;
   char *pcf_string = NULL;
   char item_code[ MAX_LINE_BUFSIZE ] = "";
   int err_code = ERROR_OK;
   int num_items = *n_items ;
   int ii = 0;
   logical item_code_is_unique = TRUE;
   char out_charx[ MAX_LINE_BUFSIZE ] = "";
   
   /*
   ** Allocate the string to be built/written
   */
   pcf_string = (char *) UF_allocate_memory(MAX_LINE_BUFSIZE,
                                            &err_code );

   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation for item code string.\n", FALSE );
      return( UF_err_operation_aborted );
   }
   
   /* 
   ** Write the item code (part_number) for the stock or component.  
   ** Check if the item code is unique.  If it is, build the materials 
   ** information for the item.
   */ 
   get_part_number_charx ( charx_count, 
                           charx_list, 
                           comp_tag,
                           out_charx );
 
   sprintf( item_code, "%s", out_charx );

   sprintf( pcf_string, COMP_ITEM_CODE_FMT, item_code );       
   status = write_string( pcf_string, pcf_stream );

   for ( ii = 0;  ii < num_items; ii++ )
   {
       if ( !strcmp ( (*item_code_char)[ii], item_code ) )
       {
           item_code_is_unique = FALSE;
           break;
       }
   }

   if ( item_code_is_unique )
   {
       *item_code_char = (char **) UF_reallocate_memory( *item_code_char, 
                                                          sizeof (char *)*(num_items +1),
                                                          &err_code );
      /* Allocate the memory necessary to hold the new item code inside 
         of the list */
      (*item_code_char)[num_items] = (char * )UF_allocate_memory( strlen( item_code ) + 1,
                                                                  &err_code );
      strcpy( (*item_code_char)[num_items], item_code );
      num_items += 1;
   }

   *n_items = num_items;   

   if( item_code_is_unique )
   {
      build_materials( charx_count, charx_list, comp_tag, material_stream );
   }

   UF_free( pcf_string );
 
   return (status);
}

/**********************************************************************
* Function Name: build_materials
* 
* Function Description: Build the Materials section for the PCF.  As 
*     components items are written to the PCF, their unique item codes 
*     (part numbers) are written to the Materials file.  The Materials 
*     file is merged with the PCF file after all of the components are 
*     written.  Search the charx list for all know material attributes 
*     and write those that are found (it is OK not to find them all for 
*     each component).
*
* Input: int charx_count - number of routing charxs 
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * material_stream - FILE stream of temporary material file
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::build_materials( int charx_count,
                            UF_ROUTE_charx_p_t charx_list,
                            tag_t comp_tag,
                            FILE * material_stream )
{
   int index = 0; 
   int status = ERROR_OK;
   char *mat_string = NULL;
   int err_code = ERROR_OK;
   char out_charx[ MAX_LINE_BUFSIZE ] = "";
    
   mat_string = (char *) UF_allocate_memory(MAX_LINE_BUFSIZE,
                                            &err_code );

   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for material string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }
   
   /*
   ** Write the item code
   */ 
   status = get_part_number_charx ( charx_count, charx_list, comp_tag, out_charx );
   sprintf( mat_string, MAT_ITEM_CODE_FMT, out_charx );
   status = write_string( mat_string, material_stream );
  
   strcpy ( out_charx, "" ); 
   /*
   ** Write the material information found in the items charactistic list
   */

   /*
   ** DESCRIPTION string
   */
   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                       UF_ROUTE_DESCRIPTION_STR, 
                                       &index);
   if ( index < 0 )
       status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                              "DB_DESCRIPTION", 
                                              &index);
   if ( status == ERROR_OK && index >= 0 ) 
   {
      sprintf(mat_string, MAT_DESC_FMT, charx_list[index].value.s_value); 
      status = write_string( mat_string, material_stream );
   }

   /*
   ** MATERIAL string
   */
   status = get_material_charx ( charx_count, charx_list, comp_tag, out_charx ); 
   
   if (status == ERROR_OK) 
   {
      sprintf( mat_string, MAT_MATERIAL_FMT, out_charx); 
      status = write_string( mat_string, material_stream );
   }
   
   /*
   ** SCHEDULE string
   */
   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                          UF_ROUTE_SCHEDULE_STR, 
                                          &index);
   
   if (status == ERROR_OK) 
   {
      sprintf( mat_string, MAT_SCHEDULE_FMT, charx_list[index].value.s_value ); 
      status = write_string(mat_string, material_stream );
   }
   
   /* 
   ** CLASS string
   */
   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                          UF_ROUTE_CLASS_STR, 
                                          &index);
   
   if (status == ERROR_OK && index > -1) 
   {
      sprintf( mat_string, MAT_CLASS_FMT, charx_list[index].value.s_value ); 
      status = write_string( mat_string, material_stream );
   }
   
   /*
   ** RATING string 
   */
   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                          UF_ROUTE_RATING_STR, &index);
   if (status == ERROR_OK) 
   {
      sprintf( mat_string, MAT_RATING_FMT, charx_list[index].value.s_value); 
      status = write_string( mat_string, material_stream );
   }
   
   //add item attributes
   for( int i = 0; i < n_pcf_item_attrs; i++ )
   {
       UF_ROUTE_find_title_in_charx( charx_count, charx_list, pcf_item_attrs[i], &index );

       if( index > -1 )
       {
           sprintf( mat_string, "    %s%d  %s\n", "ITEM-ATTRIBUTE", i, charx_list[index].value.s_value );
           write_string( mat_string, material_stream );
       }
       else
       {
           sprintf( mat_string, "    %s%d\n", "ITEM-ATTRIBUTE", i );
           write_string( mat_string, material_stream );
       }
   }
   
   UF_free( mat_string );
   
   status = ERROR_OK;     /* OK if no material information found */
   
   return( status );
}

/**********************************************************************
* Function Name: write_weight
* 
* Function Description: If found, write the weight for the given 
*        component characteristics.
*
* Input: int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * pcf_stream - channel number of PCF file
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_weight( int charx_count,
                         UF_ROUTE_charx_p_t charx_list,
                         FILE * pcf_stream )

{
   int status = ERROR_OK;
   int index = 0;
   char *pcf_string;
   int err_code = ERROR_OK;

   /*
   ** Allocate the string to be built/written
   */
   pcf_string = (char *) UF_allocate_memory(MAX_LINE_BUFSIZE,
                                            &err_code );
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for weight string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }

   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                          UF_ROUTE_WEIGHT_VALUE_STR, 
                                          &index);
   
   if (status == ERROR_OK)
   {
       if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_REAL )
       {
           sprintf(pcf_string, COMP_WEIGHT_FMT, charx_list[index].value.r_value); 
           status = write_string(pcf_string, pcf_stream );
       }
       else if( charx_list[index].type == UF_ROUTE_CHARX_TYPE_STR )
       {
           strcpy (pcf_string, charx_list[index].value.s_value); 
           status = write_string(pcf_string, pcf_stream );
       }
       else
       {
           status = ERROR_OK;   /* Weight format is not supported, User can add code to support other formats */
       }
   }
   else
   {
      status = ERROR_OK;   /* OK if weight is not found */
   }
   
   UF_free( pcf_string );
   
   return( status );
}

/**********************************************************************
* Function Name: write_skey
* 
* Function Description:
*     If found, write SKEY (symbol key) for the given component 
*     characteristics.  If a SKEY is not found, check to see if
*     the ISOGEN Component ID is found.  If a component ID is
*     not found, the component has been assigned as MISC-COMP in
*     the PCF file; assign the SKEY to the default for MISC-COMP.
*
* Input: int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * pcf_stream - FILE stream of the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_skey( int charx_count,
                       UF_ROUTE_charx_p_t charx_list,
                       FILE * pcf_stream )

{
   int status = ERROR_OK;
   int skey_index = 0; 
   int comp_index = 0;
   char *pcf_string;
   int err_code = ERROR_OK;
   
   /*
   ** Allocate the string to be built/written
   */
   pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                             &err_code );
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for skey string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }
   
   /*
   ** Find the SKEY (symbol key) for the component.  If there is no 
   ** SKEY value, it may be a GASKET and a skey is not required.  If the
   ** component has no Component ID value, assign the default SKEY for
   **  MISC-COMP.
   */
   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                          ISO_CHARX_SKEY_STR, 
                                          &skey_index);
   if( status != ERROR_OK || skey_index < 0 )
   {
      status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                             ISO_CHARX_COMP_ID_STR, 
                                             &comp_index); 
      if( status != ERROR_OK )
      {
         sprintf( pcf_string, COMP_SKEY_FMT, DEFAULT_MISC_COMP_SKEY );
      }
      else
      {
         /*
         ** If the Component ID is Gasket, it is OK to return without writing
         ** a skey
         */
         if( comp_index > -1 && strcmp( charx_list[comp_index].value.s_value, 
                                        GASKET_COMP_ID) == 0 )
         {
            UF_free( pcf_string );
            return( ERROR_OK );
         }
         else
         {
            UF_print_syslog( "Component requires an SKEY value.\n", FALSE );
            return( UF_err_operation_aborted );
         }
      }
      
   }
   else
   {
      sprintf( pcf_string, COMP_SKEY_FMT, 
               charx_list[skey_index].value.s_value ); 
   }
   
   status = write_string( pcf_string, pcf_stream );
   
   UF_free( pcf_string );
   
   return( status );
}

/**********************************************************************
* Function Name: write_center_point
* 
* Function Description: Write the center point for the given component 
*     characteristics. Find the anchor for the given component and use 
*     it is the center point. 
*
* Input: tag_t part_tag - tag of part for Isometric Drawing
*        tag_t comp_tag - tag of component
*        FILE * pcf_stream - FILE stream of the PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_center_point( tag_t part_tag,
                               tag_t comp_tag,
                               int charx_count, 
                               UF_ROUTE_charx_p_t charx_list,
                               FILE * pcf_stream )
{
   int status = ERROR_OK;
   int num_anchors = 0;
   double pos[3];
   double pos_r[3];
   tag_t *anchor_tags = NULL;
   char *pcf_string = NULL;
   int err_code = ERROR_OK;

   /*
   ** Allocate the string to built/written
   */
   pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                              &err_code );
    
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for center point string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }
   
   /*
   ** Get the anchor position and write it out as the 
   ** center point.  Ignore any additional anchors. 
   */
   ask_comp_anchors( part_tag, comp_tag, &num_anchors, &anchor_tags);

   if (num_anchors >= 1)
   {
      status = UF_ROUTE_ask_anchor_position( anchor_tags[0], pos);  
      CHECK_STATUS
		
	  //Add by CJ
	  coordinate_transform(pos, pos_r);

      /*sprintf( pcf_string, COMP_CENTER_POINT_FMT, 
               pos[0], pos[1], pos[2]);*/ 
	  sprintf( pcf_string, COMP_CENTER_POINT_FMT, 
               pos_r[0], pos_r[1], pos_r[2]); 
      status = write_string( pcf_string, pcf_stream );
   }
   else /* find center point mathematically */
   {
       int char_index = -1;
       int iso_index = -1;
       int num_ports = 0;
       double len = 0.0;
       tag_t *ports = NULL;
       double cp1[3] = {0}, cp2[3] = {0};
	   double cp1_r[3] = {0}, cp2_r[3] = {0};
       double mag = 0.0;
       double vec1[3] = {0}, vec2[3] = {0}, end1[3] = {0}, end2[3] = {0};

       UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                     ISO_CHARX_COMP_ID_STR, &iso_index );

       UF_ROUTE_ask_object_port ( comp_tag, &num_ports, &ports );

       if ( num_ports >= 2 )
       {
           UF_ROUTE_ask_port_align_vector ( ports[0], vec1 );
           UF_ROUTE_ask_port_position ( ports[0], end1 );
           UF_ROUTE_ask_port_align_vector ( ports[1], vec2 );
           UF_ROUTE_ask_port_position ( ports[1], end2 );
           UF_VEC3_unitize ( vec1, 0.0001, &mag, vec1 );
           UF_VEC3_unitize ( vec2, 0.0001, &mag, vec2 );
       }
       /*
        ** Elbow components require a center point and angle
        */
        if( iso_index > -1 && strcmp( charx_list[iso_index].value.s_value, ELBOW_COMP_ID ) == 0 )
        {
           get_lateral_butt_weld_center_point (  end1, 
                                                 end2, 
                                                 vec1, 
                                                 vec2, 
                                                 cp1 ) ;
        }
            
        /*
        ** Tee components require a center point
        */
        if( iso_index > -1 && strcmp( charx_list[iso_index].value.s_value, TEE_COMP_ID ) == 0 )
        {
            double dot_prod = 0.0;
            UF_VEC3_dot ( vec1, vec2, &dot_prod );

            if ( dot_prod < 0.0001 ) /* opposite vectors */
                UF_VEC3_midpt ( end1, end2, cp1 );

            else /* perpedicular vectors */
                get_lateral_butt_weld_center_point (  end1, 
                                                 end2, 
                                                 vec1, 
                                                 vec2, 
                                                 cp1 ) ;
        }
            
        /*
        ** Valve components require a center point
        */
        if( iso_index > -1 && strcmp( charx_list[iso_index].value.s_value, VALVE_COMP_ID ) == 0 )
        {
            UF_VEC3_midpt ( end1, end2, cp1 );
        }
        if ( iso_index > -1 && (strcmp( charx_list[iso_index].value.s_value, CROSS_COMP_ID ) == 0 || 
                                strcmp( charx_list[iso_index].value.s_value, "VALVE-ANGLE" ) == 0 ) )
        {
            double dot_prod = 0.0;
            UF_VEC3_dot ( vec1, vec2, &dot_prod );

            if ( dot_prod < -0.9 ) /* opposite vectors */
                UF_VEC3_midpt ( end1, end2, cp1 );

            else /* perpedicular vectors */
                get_lateral_butt_weld_center_point (  end1, 
                                                 end2, 
                                                 vec1, 
                                                 vec2, 
                                                 cp1 ) ;
        }
       
       UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                     "CENTER_LEN", &char_index );
       if ( char_index >= 0 )
           len = charx_list[char_index].value.r_value ;

       /* find center for pipe returns */
       if ( char_index > -1 && 
            len > 0.0 )
       {
           UF_VEC3_affine_comb ( end1, (-1.0)*len, vec1, cp1 );
           UF_VEC3_affine_comb ( end2, (-1.0)*len, vec2, cp2 );

       }
	   //add by CJ
	   coordinate_transform(cp1, cp1_r);
       /*sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                     cp1[0], cp1[1], cp1[2]);*/ 
	   sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                     cp1_r[0], cp1_r[1], cp1_r[2]); 
       write_string ( pcf_string, pcf_stream );
       
       if ( char_index > -1 )
       {
           strcpy ( pcf_string, "" );

		   //add by CJ
		   coordinate_transform(cp2, cp2_r);
		   /*sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                     cp2[0], cp2[1], cp2[2]); */
           sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
                     cp2_r[0], cp2_r[1], cp2_r[2]); 
           write_string ( pcf_string, pcf_stream );
       }

       if ( num_ports )
           UF_free ( ports );
   }
   
   UF_free( pcf_string );

   if( num_anchors != 0 )
   {
      UF_free( anchor_tags );
   }
   
   return (status);
}

/**********************************************************************
* Function Name: ask_comp_anchors
* 
* Function Description: Get the anchor tags for the given component.  
*
* Input: tag_t part_tag - tag of part
*        tag_t comp_tag - tag of component
* Output: int * num_anchors - number of anchors
*         tag_t ** anchor_tags - array of anchor tags
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::ask_comp_anchors(tag_t part_tag, tag_t comp_tag, 
                            int *num_anchors, tag_t **anchor_tags)
{
   int status = ERROR_OK;
   tag_t anchor_tag = NULL_TAG;
   tag_t part_occ = NULL_TAG;
   int max_anchors_per_component = 2;  /* is this true? */
   int err_code = 0;

   *anchor_tags = (tag_t *)UF_allocate_memory( max_anchors_per_component * sizeof(tag_t),
                                      &err_code );
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for center point string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }

   /*
   ** Cycle thru all of the anchors in the part.  Find the parent of the
   ** anchor and match the parent with the component 
   */ 
   do
   {
      UF_OBJ_cycle_objs_in_part( part_tag, 
                                 UF_route_part_anchor_type,
                                 &anchor_tag );

      if( anchor_tag == NULL_TAG )
      {
         break;
      }

      if( UF_ASSEM_is_occurrence( anchor_tag ) )
      {
         status = UF_ASSEM_ask_parent_component( anchor_tag, 
                                                 &part_occ );
         CHECK_STATUS

    
         if( part_occ == comp_tag)
         {
            (*anchor_tags)[(*num_anchors)] = anchor_tag;
            *num_anchors += 1;
         }
      }
   }while( anchor_tag != NULL_TAG );
   
   return( status );
}

/**********************************************************************
* Function Name: write_angle
* 
* Function Description: If found, write the angle for the given 
*     component characteristics.
*
* Input: int charx_count - number of charxs
*        UF_ROUTE_charx_p_t charx_list - array of charxs
*        FILE * pcf_stream - FILE stream of PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_angle( int charx_count,
                        UF_ROUTE_charx_p_t charx_list,
                        FILE * pcf_stream )

{
   int status = ERROR_OK;
   int index = 0; 
   char *pcf_string;
   int err_code = 0;

   /*
   ** Allocate the string to be built/written
   */
   pcf_string = (char *) UF_allocate_memory( MAX_LINE_BUFSIZE,
                                              &err_code );
   if( err_code != ERROR_OK )
   {
      UF_print_syslog( "Memory allocation error for angle string.\n", 
                       FALSE );
      return( UF_err_operation_aborted );
   }

   status = UF_ROUTE_find_title_in_charx( charx_count, charx_list,
                                          UF_ROUTE_ANGLE_STR, &index );
   
   if( status == ERROR_OK )
   { 
      sprintf( pcf_string, COMP_ANGLE_FMT, 
               charx_list[index].value.r_value); 
      status = write_string( pcf_string, pcf_stream );
   }
   else
   {
      status = ERROR_OK;       /* OK if angle not found */
   }
   
   UF_free( pcf_string );
   
   return (status);
}

/**********************************************************************
* Function Name: write_support_coords
* 
* Function Description: Write the coords for support component 
*
* Input:  comp_tag -  component tag
*         pcf_stream - FILE stream of PCF
* Returns        
***********************************************************************/
void pcf_generation::write_support_coords ( tag_t part_tag,
                                   tag_t comp_tag,
                                   FILE * pcf_stream )
{
    int num_objs = 0;
    tag_t *objs = NULL;
    double pos[3]={0};
	double pos_abs[3]={0};

    ask_comp_anchors( part_tag, comp_tag, 
                      &num_objs, 
                      &objs);

    if ( num_objs > 0 &&  objs[0] != NULL_TAG )
    {
        UF_ROUTE_ask_anchor_position( objs[0], pos_abs );
    }
    else
    {
        UF_ROUTE_ask_object_port ( comp_tag, &num_objs, &objs );

        if ( num_objs > 0 && objs[0] != NULL_TAG )
        {
            UF_ROUTE_ask_port_position ( objs[0], pos_abs );
        }
    }
    UF_free( objs );

	//*****Add by CJ******coordinate_transform*******
	coordinate_transform(pos_abs, pos);
 
    int charx_count = 0, index = 0;
    double size = 0.0;
    UF_ROUTE_charx_p_t charx_list     = NULL;

    int status = UF_ROUTE_ask_characteristics( comp_tag, 
                                           UF_ROUTE_CHARX_TYPE_ANY,
                                           &charx_count, &charx_list);

    CHECK_STATUS

    status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                           UF_ROUTE_NPS_STR, &index);

    CHECK_STATUS

    if ( index > -1 )
    {
         size = charx_list[index].value.r_value;
    }

    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
    sprintf ( pcf_string, "    CO-ORDS %.4f %.4f %.4f %.4f\n",
              pos[0], pos[1], pos[2], size );

    write_string( pcf_string, pcf_stream );

    if ( charx_count )
        UF_ROUTE_free_charx_array ( charx_count, charx_list );
}
/**********************************************************************
* Function Name: write_olet_center_and_branch_points
* 
* Function Description: Write the center point and branch point for 
*     the given outlet component.
*
* Input: tag_t comp_tag - tag of component 
*        FILE * pcf_stream - FILE stream of PCF
* 
* Returns: 
***********************************************************************/
void pcf_generation::write_olet_center_and_branch_points ( tag_t comp_tag,
                                       FILE * pcf_stream )
{
    logical is_fixture  = FALSE;
    int     num_ports   = 0;
    int     index       = 0;
    int     charx_count = 0;
    int     status      = 0;
    double  center[3]   = {0.0, 0.0, 0.0};
    double  branch[3]   = {0.0, 0.0, 0.0};
	double  center_abs[3]   = {0.0, 0.0, 0.0};
    double  branch_abs[3]   = {0.0, 0.0, 0.0};
	double  vector1[3] = {0.0, 0.0, 0.0};
	double  vector2[3] = {0.0, 0.0, 0.0};
	double  vector3[3] = {0.0, 0.0, 0.0};
	int is_parallel = 0;
    tag_t   *ports      = NULL;
    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
    UF_ROUTE_charx_p_t charx_list     = NULL;

    UF_ROUTE_ask_object_port ( comp_tag, &num_ports, &ports );

    if ( num_ports < 2 )
    {
        UF_free ( ports );
        return ;
    }

	if( num_ports == 3 )
	{
		UF_ROUTE_ask_port_align_vector(ports[0], vector1);
		UF_ROUTE_ask_port_align_vector(ports[1], vector2);
		UF_ROUTE_ask_port_align_vector(ports[2], vector3);
		UF_VEC3_is_parallel(vector1, vector2, 0.05, &is_parallel);
		if(is_parallel)
		{
			UF_ROUTE_ask_port_position ( ports[0], center_abs );
			UF_ROUTE_ask_port_position ( ports[2], branch_abs );
		}else
		{
			UF_VEC3_is_parallel(vector1, vector3, 0.05, &is_parallel);
			if(is_parallel)
			{
				UF_ROUTE_ask_port_position ( ports[0], center_abs );
				UF_ROUTE_ask_port_position ( ports[1], branch_abs );
			}else
			{
				UF_ROUTE_ask_port_position ( ports[1], center_abs );
				UF_ROUTE_ask_port_position ( ports[0], branch_abs );
			}
		}
	}
   
	if(num_ports == 2)
	{
		UF_ROUTE_is_port_fixture_port ( ports[0], &is_fixture );

		if ( !is_fixture )/* may be second port is fixture port */
		{
			UF_ROUTE_is_port_fixture_port ( ports[1], &is_fixture );

			if ( is_fixture )
			{
				UF_ROUTE_ask_port_position ( ports[1], center_abs );
				UF_ROUTE_ask_port_position ( ports[0], branch_abs );
			}
			else /* none of the ports are fixture ports*/
			{
				UF_free ( ports );
				return;
			}
		}
		else /* first port is fixture port */
		{
			UF_ROUTE_ask_port_position ( ports[0], center_abs );
			UF_ROUTE_ask_port_position ( ports[1], branch_abs );
		}
	}
    UF_free( ports );

	//***Add by CJ*****coordinate_transform*****
	coordinate_transform(center_abs, center);
	coordinate_transform(branch_abs, branch);

    status = UF_ROUTE_ask_characteristics( comp_tag, 
                                           UF_ROUTE_CHARX_TYPE_ANY,
                                           &charx_count, &charx_list);

    status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
                                           UF_ROUTE_NPS_STR, &index);

    CHECK_STATUS

    sprintf ( pcf_string, COMP_CENTER_POINT_FMT,
              center[0], center[1], center[2] );

    write_string ( pcf_string, pcf_stream );

    sprintf( pcf_string, COMP_BRANCH1_POINT_FMT, 
             branch[0], branch[1], branch[2], 
             charx_list[index].value.r_value );

    write_string ( pcf_string, pcf_stream );

    strcpy ( pcf_string, "" );
    
    if ( charx_count )
        UF_ROUTE_free_charx_array ( charx_count, charx_list );
}

/* *********************************************************************
* Function Name: find_start_and_end_port
* 
* Function Description:  find the port with DIAMETER defined as NPS OUT and use that as end port
* 
* Input: tag_t port_tags -  array of port tags
* 
* Output:
*        tag_t start_port  - initial start port
*        tag_t end_port    - initial end port
* 
* 
* Returns:
*     ERROR_OK if OK
*     error code (ERROR_raise) if error 
*************************************************************************/ 
void pcf_generation::find_start_and_end_port(tag_t* port_tags, tag_t* start_port, tag_t* end_port )
{
	UF_ATTR_info_t info;
	logical has_attr = false;

	UF_ATTR_get_user_attribute_with_title_and_type(port_tags[0], "R$$NPS",UF_ATTR_any, UF_ATTR_NOT_ARRAY, &info, &has_attr);
	if(has_attr)
	{
		if(!strcmp(info.string_value, "NPS_OUT")){
			*end_port   = port_tags[0];
			*start_port = port_tags[1];
		}else
		{
			*end_port   = port_tags[1];
			*start_port = port_tags[0];
		}
	}else
	{
		UF_ATTR_get_user_attribute_with_title_and_type(port_tags[0], "R$$DIAMETER",UF_ATTR_any, UF_ATTR_NOT_ARRAY, &info, &has_attr);
		if(has_attr)
		{
			if(!strcmp(info.string_value, "NPS_OUT")){
				*end_port   = port_tags[0];
				*start_port = port_tags[1];
			}else
			{
				*end_port   = port_tags[1];
				*start_port = port_tags[0];
			}
		}
	}
	UF_ATTR_free_user_attribute_info_strings(&info);
}

/* *********************************************************************
*  Function Name: find_nps_branch_value
*  
*  Function Description: if either port has the attribute NPS_BRANCH or NPS_OUT, use that as outDiameter
*  
*  Input: tag_t port_tags   -  array of port tags
*         double inDiameter -  NPS value of component
* 
*  Returns:
*         double outDiameter - value found in attribute for NPS_OUT or NPS_BRANCH
*  
*************************************************************************/ 
double pcf_generation::find_nps_branch_value( tag_t* port_tags, double inDiameter )
{
    /* assign the outDiameter to be the same as the inDiameter.  
       It will get overwritten if one of the ports has an appropriate attribute.   */
    double outDiameter = inDiameter;

    for ( int iPort = 0; iPort < 2;  ++iPort)
    {   
        int port_charx_count = 0;
        UF_ROUTE_charx_p_t port_charx_list = NULL;
        int char_status = UF_ROUTE_ask_characteristics ( port_tags[iPort], 
                                                         UF_ROUTE_CHARX_TYPE_ANY,
                                                         &port_charx_count, 
                                                         &port_charx_list );
       
        if ( port_charx_count )
        {      
            int char_index = -1;
            char_status = find_nps_branch_index ( port_charx_count, 
                                                port_charx_list, 
                                                &char_index );
       
            if ( char_status == ERROR_OK && char_index >= 0 )
            {
                if ( port_charx_list[char_index].type == UF_ROUTE_CHARX_TYPE_INT )
                {
                    outDiameter = port_charx_list[char_index].value.i_value;
                }
                else if  ( port_charx_list[char_index].type == UF_ROUTE_CHARX_TYPE_REAL )
                {
                    outDiameter = port_charx_list[char_index].value.r_value;
                }
            }
        }
    }

    return outDiameter;

}

/* *********************************************************************
*  Function Name: determine_in_diameter
*  
*  Function Description: return the value for attribute named NPS
*  
*  Input: tag_t comp_tag    -  tag of the component
* 
*  Returns:
*         double inDiameter - value found in attribute for NPS
*  
*************************************************************************/ 
double pcf_generation::determine_in_diameter ( tag_t comp_tag ) 
{
    
    double inDiameter = 0.0;
        
    int charx_count = 0;
    UF_ROUTE_charx_p_t charx_list = NULL;
    int index = -1;


    /* Get the charxs for the component */
    int status = UF_ROUTE_ask_characteristics( comp_tag, 
                                               UF_ROUTE_CHARX_TYPE_ANY,
                                               &charx_count, &charx_list);
    
    /* look for the NPS attribute */
    status = UF_ROUTE_find_title_in_charx( charx_count, 
                                               charx_list, 
                                               UF_ROUTE_NPS_STR, 
                                               &index);
    
    /* if the attribute title 'NPS' was found, return the value of the attribute */
    if ( status == ERROR_OK && index >= 0 )
    {
        if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_INT )
        {
            inDiameter = charx_list[index].value.i_value;
        }
        else if  ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_REAL )
        {
            inDiameter = charx_list[index].value.r_value;
        }else if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_STR )
		{
			inDiameter = atof(charx_list[index].value.s_value);
		}
    }else{
		status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, "OD", &index);
		if ( status == ERROR_OK && index >= 0 )
		{
			if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_INT )
			{
				inDiameter = charx_list[index].value.i_value;
			}
			else if  ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_REAL )
			{
				inDiameter = charx_list[index].value.r_value;
			}else if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_STR )
			{
				inDiameter = atof(charx_list[index].value.s_value);
			}
		}
	}

    /* return default value if not found */
    return inDiameter;
   
}

/* *********************************************************************
*  Function Name: determine_out_diameter
*  
*  Function Description: return the value for attribute specifying NPS_BRANCH or its aliases.
*                        If these attributes are not found on the component, check
*                        the component's ports.
*  
*  Input: tag_t comp_tag    -  tag of the component
*         double inDiameter -  NPS value
* 
*  Returns:
*         double outDiameter - value found in attribute for NPS_OUT or NPS_BRANCH
* 
*************************************************************************/ 
double pcf_generation::determine_out_diameter ( tag_t comp_tag, double inDiameter ) 
{
    
    /* assign the outDiameter to be the same as the inDiameter.  It will get overwritten if appropriate. */
    double outDiameter = inDiameter;

    /* check the main component for the attribute */
    int charx_count = 0;
    UF_ROUTE_charx_p_t charx_list = NULL;
    int status = UF_ROUTE_ask_characteristics( comp_tag, 
                                               UF_ROUTE_CHARX_TYPE_ANY,
                                               &charx_count, &charx_list);
    /* check for R$NPS_OUT */
    int typeOfAttribute;
    UF_ATTR_find_attribute ( comp_tag, UF_ATTR_any, "R$NPS_OUT", &typeOfAttribute );

    /* if R$NPS_OUT was not found,  check for NPS_OUT */
    if( typeOfAttribute == 0 )
    {
        UF_ATTR_find_attribute ( comp_tag, UF_ATTR_any, "NPS_OUT", &typeOfAttribute );
    }

    /* if NPS_OUT was not found,  check for NPS_BRANCH */
    if( typeOfAttribute == 0 )
    {
        UF_ATTR_find_attribute ( comp_tag, UF_ATTR_any, "NPS_BRANCH", &typeOfAttribute );
    }

    /* if either attribute was found, then return the value found at that attribute */
    if ( typeOfAttribute > 0 )
    {
         int index = -1;
         status = find_nps_branch_index ( charx_count, 
                                        charx_list, 
                                        &index );
       
         /* if the attribute title 'NPS' was found, return the value of the attribute */
         if ( status == ERROR_OK && index >= 0 )
         {
             if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_INT )
             {
                 outDiameter = charx_list[index].value.i_value;
             }
             else if  ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_REAL )
             {
                 outDiameter = charx_list[index].value.r_value;
             }else if ( charx_list[index].type == UF_ROUTE_CHARX_TYPE_STR )
			 {
				 outDiameter = atof(charx_list[index].value.s_value);
			 }
         }

         return outDiameter;
     }

    /* if a value was not found on the main component, check its ports. */
    int num_ports;
    tag_t *port_tags = NULL;

    UF_ROUTE_ask_object_port( comp_tag, &num_ports, &port_tags );

    if ( num_ports == 2 )
    {
        outDiameter = find_nps_branch_value( port_tags, inDiameter );
    }

    return outDiameter;
   
}

/* *********************************************************************
*  Function Name: write_flat_direction
*  
*  Function Description: determine and write the value for flat direction to the pcf file
*  
*  Input: tag_t start_port    -  tag of the start port
*         tag_t end_port      -  tag of the end port
*         FILE * pcf_stream   -  the pcf file 
* 
*************************************************************************/ 
void pcf_generation::write_flat_direction ( tag_t start_port, tag_t end_port, FILE * pcf_stream )
{
    int status = ERROR_OK;
    double end_pt1[3] = {0};
    double end_pt2[3] = {0};

    status = UF_ROUTE_ask_port_position( start_port, end_pt1);
    CHECK_STATUS
    status = UF_ROUTE_ask_port_position ( end_port, end_pt2);
    CHECK_STATUS
   
    double end_vec[3] = {0.0, 0.0, 0.0}; 
    end_vec[0] = end_pt2[0]-end_pt1[0];
    end_vec[1] = end_pt2[0]-end_pt1[1];
    end_vec[2] = end_pt2[0]-end_pt1[2];
    
    double vec1[3]      = {0};
    double cross_vec[3] = {0.0, 0.0, 0.0};
    double flat_angle   = 0.0;
    UF_ROUTE_ask_port_align_vector ( start_port, vec1 ); 
    UF_VEC3_cross ( vec1, end_vec, cross_vec );
    UF_VEC3_angle_between ( vec1, end_vec, cross_vec, &flat_angle );
    
    flat_angle = flat_angle * RADEG;
    
    char flat_string[5]="";
    if ( flat_angle > 180.0 )
    {
         strcpy ( flat_string, "UP" );
    }
    else
    {
        strcpy ( flat_string, "DOWN" );
    }
    
    char* pcf_string = (char *) UF_allocate_memory(MAX_LINE_BUFSIZE,  &status );
    sprintf ( pcf_string, "    FLAT-DIRECTION %s\n", flat_string );
    write_string ( pcf_string, pcf_stream );
    UF_free( pcf_string );
}

/**********************************************************************
* Function Name: write_end_and_branch_points
* 
* Function Description: Write the end points and any branch points for 
*     the given component.
*
* Input: tag_t part_tag - tag of part for Isometric Drawing
*        tag_t comp_tag - tag of component 
*        FILE * pcf_stream - FILE stream of PCF
* 
* Returns: ERROR_OK or UF_err_operation_aborted
***********************************************************************/
int pcf_generation::write_end_and_branch_points(tag_t part_tag,
                                       tag_t comp_tag,
                                       FILE * pcf_stream )
{
	int status = ERROR_OK;
	int index = 0;
	int title_index = -1;
	int charx_count = 0;
	double end_pt1[3] = {0};
	double end_pt2[3] = {0};
	double anchor_pos1[3] = {0};
	double anchor_pos2[3] = {0};
	double branch_pt1[3] = {0};
	double branch_pt2[3] = {0};
	double end_pt1_r[3] = {0};
	double end_pt2_r[3] = {0};
	double anchor_pos1_r[3] = {0};
	double anchor_pos2_r[3] = {0};
	double branch_pt1_r[3] = {0};
	double branch_pt2_r[3] = {0};
	double distance_1 = 0;
	double distance_2 = 0;
	tag_t *port_tags = NULL;
	int num_ports = 0;
	tag_t *anchor_tags = NULL;
	int num_anchors = 0;
	char *pcf_string = NULL;
	logical branch_pt1_assigned = FALSE;
	logical branch_pt2_assigned = FALSE;
	UF_ROUTE_charx_p_t charx_list = NULL;
	double distance = 0.0;
	char message[ MAX_LINE_BUFSIZE ]="";
	int err_code = ERROR_OK;
	double tol = 0.0;
	double vec1[3] = {0}, vec2[3] = {0}, vec3[3] = {0}, dot;
	tag_t start_port = NULL_TAG;
	tag_t end_port = NULL_TAG;
	double cross_vec[3] = {0.0, 0.0, 0.0};
	double tee_angle = 0.0;
	logical lateral_butt_weld = FALSE;
	double butt_center[3] = {0.0, 0.0, 0.0};
	double butt_center_r[3] = {0.0, 0.0, 0.0};

	UF_ATTR_info_t cross_info;
	logical has_nps_out = false;

	UF_ATTR_info_t name_info;
	logical has_name_attr = false;
	logical is_X0002 = false;

	UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_any , UF_ATTR_NOT_ARRAY, &name_info, &has_name_attr);
	if(has_name_attr)
	{
		if(strstr(name_info.string_value, "X0002") != NULL)
		{
			is_X0002 = true;
		}
	}
	UF_ATTR_free_user_attribute_info_strings(&name_info);

	/*
	** Get the charxs for the component
	*/
	status = UF_ROUTE_ask_characteristics( comp_tag, 
											UF_ROUTE_CHARX_TYPE_ANY,
											&charx_count, &charx_list);
	CHECK_STATUS

	if( status != ERROR_OK )
	{
		sprintf( message, "Failed to get charx for component %u\n", comp_tag );
       
		UF_print_syslog( message, FALSE );

		return( UF_err_operation_aborted );
	}

	status = UF_ROUTE_find_title_in_charx ( charx_count, charx_list,
													ISO_CHARX_COMP_ID_STR,
													&title_index );
   
	/*
	** Find the component ports and use them as the end points and
	** branch points. 
	*/
	UF_ROUTE_ask_object_port( comp_tag, &num_ports, &port_tags );
	switch (num_ports)
	{
		/* 
		** If one port is found, use it and one of the component anchors
		** as the end points.  Use the component INSLEN to determine which
		** anchor to use as the second end point.
		** This special processing for CAPS may be an error in the way
		** CAP components are created (do the really need two anchors?)  
		** Note that butt-welded CAPS only have one anchor so it is used
		** as the end point.
		*/
		case 1:
			status = UF_ROUTE_ask_port_position( port_tags[0], end_pt1 );
			CHECK_STATUS
			UF_ROUTE_ask_port_align_vector ( port_tags[0], vec1 ); 
      
			ask_comp_anchors( part_tag, comp_tag, 
							&num_anchors, 
							&anchor_tags);
      
			switch(num_anchors)
			{
			case 1:
				status = UF_ROUTE_ask_anchor_position( anchor_tags[0], 
														end_pt2);  
				CHECK_STATUS
				break;
         
			case 2:
				status = UF_ROUTE_ask_anchor_position( anchor_tags[0], 
														anchor_pos1); 
				CHECK_STATUS

				status = UF_ROUTE_ask_anchor_position( anchor_tags[1], 
														anchor_pos2); 
				CHECK_STATUS

				if (charx_list[title_index].value.s_value != NULL && !strcmp ( charx_list[title_index].value.s_value, "CAP" ) )
				{
					double direction[3] = {0.0, 0.0, 0.0};
					point_direction(end_pt1, anchor_pos1, direction);
					UF_VEC3_dot(vec1, direction, &dot);
					if(dot < 0)
					{
						copy_position(anchor_pos1, end_pt2);
						break;
					}else
					{
						copy_position(anchor_pos2, end_pt2);
						break;
					}
				}
         
				/*
				** Get the inset length of the component and use it to
				** determine which anchor point is closest to the stock.
				*/
				status = UF_ROUTE_find_title_in_charx( charx_count, 
														charx_list, 
														UF_ROUTE_INSLEN_STR, 
														&index);
				if (status != ERROR_OK)
				{
					UF_print_syslog( "INSLEN charx not found for object\n", 
									FALSE );
					return( UF_err_operation_aborted );
				}
               
				/* Function returns void ?!? */
				UF_VEC3_distance( end_pt1, anchor_pos1, &distance );

				/* Get the routing tolerance so that we can compare positions */
				UF_ROUTE_ask_length_tolerance( &tol );
               
				if( fabs( distance - charx_list[index].value.r_value ) <= tol )
				{
					copy_position(anchor_pos2, end_pt2);
					break;
				}
               
				UF_VEC3_distance( end_pt1, anchor_pos2, &distance );

				if( fabs( distance - charx_list[index].value.r_value ) <= tol )
				{
					copy_position(anchor_pos1, end_pt2);
					break;
				}
         
				/* 
				** At this point, the second end point has not been set.
				** There is likely a break in the cap/stock connection and
				** the PCF will be invalid.  End the PCF creation. 
				*/
				UF_print_syslog( "Invalid Route Assemlby at Cap\n", FALSE );
				return( UF_err_operation_aborted ); 
			/*NOTREACHED*/
			break;
         
			default:
			sprintf( message, "Failed_to get end points for component %u\n", comp_tag );
			UF_print_syslog( message, FALSE );
			break;
			}

		if( anchor_tags != NULL )
		{
			UF_free(anchor_tags);
		}
		break; /* One port found */
      
		/*
		** If two ports are found, use them as the component end points.
		*/        
		case 2:
         
			start_port = port_tags[0];
			end_port = port_tags[1];

			if ( !status && title_index > -1 )
			{
				int port_charx_count = 0;
				int char_status  = 0, char_index = 0;
				UF_ROUTE_charx_p_t port_charx_list = NULL;

				//if (charx_list[title_index].value.s_value != NULL && 
				//	!strcmp ( charx_list[title_index].value.s_value, "FLANGE" ) )
				//{
				//	/* find the flange connection and use it as end point 2 */

				//	char_status = UF_ROUTE_ask_characteristics ( port_tags[0], 
				//											UF_ROUTE_CHARX_TYPE_ANY,
				//											&port_charx_count, 
				//											&port_charx_list );

				//	if ( port_charx_count && !char_status )
				//	{
				//		char_status = UF_ROUTE_find_title_in_charx ( port_charx_count, 
				//									port_charx_list,
				//									"CONNECTION_TYPE",
				//									&char_index );

				//		if ( !char_status && 
				//			port_charx_list[char_index].value.s_value != NULL &&
				//			strcmp ( port_charx_list[char_index].value.s_value, "FLANGE" ) )
				//		{
				//			/* this should be the end port, butt weld, slip on or lap
				//			joint port */
				//			end_port = port_tags[0];
				//			start_port = port_tags[1];
				//		}
				//		UF_ROUTE_free_charx_array ( port_charx_count, port_charx_list );                                    
				//	}

				//}
				//else 
				if ( charx_list[title_index].value.s_value != NULL &&
						!strcmp ( charx_list[title_index].value.s_value, "REDUCER-CONCENTRIC") ||
						!strcmp ( charx_list[title_index].value.s_value, "REDUCER-ECCENTRIC")    )
				{
					find_start_and_end_port (port_tags, &start_port, &end_port );
				}
				else if ( charx_list[title_index].value.s_value != NULL &&
						!strcmp ( charx_list[title_index].value.s_value, "ELBOW") )
				{
					find_start_and_end_port (port_tags, &start_port, &end_port );
				}
			}
			find_start_and_end_port (port_tags, &start_port, &end_port );
			// Ask end points from components start and end ports 
			// to write in pcf file.
			status = UF_ROUTE_ask_port_position( start_port, end_pt1); 
			CHECK_STATUS

			status = UF_ROUTE_ask_port_position( end_port, end_pt2);
			CHECK_STATUS
         
			break;
         
			/*
			** If three ports are found, use two of them as the end points and
			** one for the branch point.  This is the case with a TEE
			** component.
			*/
		case 3:
         
			UF_ROUTE_ask_port_align_vector ( port_tags[0], vec1 ); 
			UF_ROUTE_ask_port_align_vector ( port_tags[1], vec2 );
			UF_ROUTE_ask_port_align_vector ( port_tags[2], vec3 );

			UF_VEC3_dot ( vec1, vec2, &dot );

			if ( dot < -0.9 ) /* 1 & 2 are opposite */ 
			{
				UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
				UF_ROUTE_ask_port_position(port_tags[1], end_pt2);
				UF_ROUTE_ask_port_position(port_tags[2], branch_pt1);

				UF_VEC3_cross ( vec1, vec3, cross_vec );
				UF_VEC3_angle_between ( vec1, vec3, cross_vec, &tee_angle );
				if ( tee_angle * RADEG < 89.00 || tee_angle * RADEG > 91.00 )
				{
					lateral_butt_weld = TRUE;
					tee_angle = tee_angle * RADEG;
					get_lateral_butt_weld_center_point ( end_pt1, branch_pt1, 
														vec1, vec3, butt_center );
				}
			}
			else
			{
				UF_VEC3_dot ( vec1, vec3, &dot );

				if ( dot < -0.9 ) /* 1 & 3 are opposite */
				{
					UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
					UF_ROUTE_ask_port_position(port_tags[1], branch_pt1);

					UF_VEC3_cross ( vec1, vec2, cross_vec );
					UF_VEC3_angle_between ( vec1, vec2, cross_vec, &tee_angle );
					if ( tee_angle * RADEG < 89.00 || tee_angle * RADEG > 91.00 )
					{
						lateral_butt_weld = TRUE;
						tee_angle = tee_angle * RADEG;
						get_lateral_butt_weld_center_point ( end_pt1, branch_pt1, 
															vec1, vec2, butt_center );
					}
				}
				else /* 2 & 3 are opposite */
				{
					UF_ROUTE_ask_port_position(port_tags[1], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
					UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);

					UF_VEC3_cross ( vec1, vec2, cross_vec );
					UF_VEC3_angle_between ( vec1, vec2, cross_vec, &tee_angle );
					if ( tee_angle * RADEG < 89.00 || tee_angle * RADEG > 91.00 )
					{
						lateral_butt_weld = TRUE;
						tee_angle = tee_angle * RADEG;
						get_lateral_butt_weld_center_point ( end_pt1, branch_pt1, 
															vec2, vec1, butt_center );
					}
				}
             
			}
			branch_pt1_assigned = TRUE;
			break;
         
			/*
			** If four ports are found, use two as the end points and two as
			** branch points.  This is the case with a CROSS component.
			*/ 
		case 4:
		UF_ROUTE_ask_port_align_vector ( port_tags[0], vec1 ); 
		UF_ROUTE_ask_port_align_vector ( port_tags[1], vec2 );
		UF_ROUTE_ask_port_align_vector ( port_tags[2], vec3 );

		UF_VEC3_dot ( vec1, vec2, &dot );

		if (dot < -0.9) /* 1 & 2 are opposite */
		{
			status = UF_ATTR_get_user_attribute_with_title_and_type(port_tags[0], "R$$NPS", UF_ATTR_any, UF_ATTR_NOT_ARRAY, &cross_info, &has_nps_out);
			if (has_nps_out)
			{
				if (!strcmp(cross_info.string_value, "NPS_BRANCH")) {
					UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
					UF_ROUTE_ask_port_position(port_tags[1], branch_pt2);
					UF_ROUTE_ask_port_position(port_tags[2], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
				}
				else if (!strcmp(cross_info.string_value, "NPS")) {
					UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[1], end_pt2);
					UF_ROUTE_ask_port_position(port_tags[2], branch_pt1);
					UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
				}
			}
			else
			{
				UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
				UF_ROUTE_ask_port_position(port_tags[1], branch_pt2);
				UF_ROUTE_ask_port_position(port_tags[2], end_pt1);
				UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
				distance_1 = line_length(branch_pt1, branch_pt2);
				distance_2 = line_length(end_pt1, end_pt2);
				if (distance_1 > distance_2)
				{
					UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[1], end_pt2);
					UF_ROUTE_ask_port_position(port_tags[2], branch_pt1);
					UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
				}
			}
			UF_ATTR_free_user_attribute_info_strings(&cross_info);
		}
		else
		{
			UF_VEC3_dot(vec1, vec3, &dot);

			if (dot < -0.9) /* 1 & 3 are opposite */
			{
				status = UF_ATTR_get_user_attribute_with_title_and_type(port_tags[0], "R$$NPS", UF_ATTR_any, UF_ATTR_NOT_ARRAY, &cross_info, &has_nps_out);
				if (has_nps_out)
				{
					if (!strcmp(cross_info.string_value, "NPS_BRANCH")) {
						UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], branch_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
					}
					else if (!strcmp(cross_info.string_value, "NPS")) {
						UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
					}
				}
				else
				{
					UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
					UF_ROUTE_ask_port_position(port_tags[2], branch_pt2);
					UF_ROUTE_ask_port_position(port_tags[1], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
					distance_1 = line_length(branch_pt1, branch_pt2);
					distance_2 = line_length(end_pt1, end_pt2);
					if (distance_1 > distance_2)
					{
						UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
					}
				}
				UF_ATTR_free_user_attribute_info_strings(&cross_info);
			}
			else /* 2 & 3 are opposite */
			{
				status = UF_ATTR_get_user_attribute_with_title_and_type(port_tags[0], "R$$NPS", UF_ATTR_any, UF_ATTR_NOT_ARRAY, &cross_info, &has_nps_out);
				if (has_nps_out)
				{
					if (!strcmp(cross_info.string_value, "NPS_BRANCH")) {
						UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
					}
					else if (!strcmp(cross_info.string_value, "NPS")) {
						UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], branch_pt2);
					}
				}
				else
				{
					UF_ROUTE_ask_port_position(port_tags[0], branch_pt1);
					UF_ROUTE_ask_port_position(port_tags[3], branch_pt2);
					UF_ROUTE_ask_port_position(port_tags[1], end_pt1);
					UF_ROUTE_ask_port_position(port_tags[2], end_pt2);
					distance_1 = line_length(branch_pt1, branch_pt2);
					distance_2 = line_length(end_pt1, end_pt2);
					if (distance_1 > distance_2)
					{
						UF_ROUTE_ask_port_position(port_tags[0], end_pt1);
						UF_ROUTE_ask_port_position(port_tags[3], end_pt2);
						UF_ROUTE_ask_port_position(port_tags[1], branch_pt1);
						UF_ROUTE_ask_port_position(port_tags[2], branch_pt2);
					}
				}
				UF_ATTR_free_user_attribute_info_strings(&cross_info);
			}
		}
		branch_pt1_assigned = TRUE;
		branch_pt2_assigned = TRUE;
		break;

         
		default:
			sprintf( message, "Invalid number of ports for component %u\n", comp_tag );
			UF_print_syslog( message, FALSE );
			num_ports = 0;
			status = UF_err_operation_aborted;
			break;
	}

	//******Add by CJ********coordinate transform
	coordinate_transform(end_pt1, end_pt1_r);
	coordinate_transform(end_pt2, end_pt2_r);
	coordinate_transform(branch_pt1, branch_pt1_r);
	coordinate_transform(branch_pt2, branch_pt2_r);
	coordinate_transform(butt_center, butt_center_r);
    
	if (num_ports != 0)
	{
     
		int is_comp_branch = FALSE;
		UF_free(port_tags);        

		double inDiameter  = determine_in_diameter  ( comp_tag );
		double outDiameter = determine_out_diameter ( comp_tag, inDiameter );

		if(is_X0002)
		{
			pointinfo point_temp;
			point_temp.end_point[0] = end_pt1_r[0];
			point_temp.end_point[1] = end_pt1_r[1];
			point_temp.end_point[2] = end_pt1_r[2];
			point_temp.diameter = inDiameter;
			control_point.push_back(point_temp);
			point_temp.end_point[0] = end_pt2_r[0];
			point_temp.end_point[1] = end_pt2_r[1];
			point_temp.end_point[2] = end_pt2_r[2];
			point_temp.diameter = outDiameter;
			control_point.push_back(point_temp);
		}

		/* Write the end points */
		//write_end_points( end_pt1, end_pt2, inDiameter, outDiameter, pcf_stream );

		/* for TEE and CROSS, endpoint diameter should be same */
		if ( title_index != -1 && charx_list[title_index].value.s_value  != NULL && (!strcmp ( charx_list[title_index].value.s_value, "TEE") || !strcmp ( charx_list[title_index].value.s_value, "CROSS")) )
		{
			write_end_points( end_pt1_r, end_pt2_r, inDiameter, inDiameter, pcf_stream );
		}else
		{
			write_end_points( end_pt1_r, end_pt2_r, inDiameter, outDiameter, pcf_stream );
		}

		/* for eccentric reducers, write flat direction data */
		if ( title_index != -1 &&
			charx_list[title_index].value.s_value  != NULL && 
			!strcmp ( charx_list[title_index].value.s_value, "REDUCER-ECCENTRIC") )
		{
			write_flat_direction ( start_port, end_port, pcf_stream );
		}

		/*
		** If at least one branch point was assigned, write the branch points.
		*/
		if (branch_pt1_assigned ) 
		{
			/*
			** Allocate the string to be built/written
			*/
			pcf_string = (char *) UF_allocate_memory(MAX_LINE_BUFSIZE,
													&err_code );
			if( err_code != ERROR_OK )
			{
				UF_print_syslog( "Memory allocation error for branch point string.\n", 
								FALSE );
				return( UF_err_operation_aborted );
			}
        
			/*
			** Include the size of the component with each branch point
			*/
			/* Attribute may be NPS_BRANCH or R$NPS_BRANCH, so check for both */
			UF_ATTR_find_attribute ( comp_tag, UF_ATTR_any, "R$NPS_BRANCH", &is_comp_branch );         
			if( !is_comp_branch )
			{
				UF_ATTR_find_attribute ( comp_tag, UF_ATTR_any, "NPS_BRANCH", &is_comp_branch );
			}
       
			if ( is_comp_branch )
				status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
													UF_ROUTE_NPS_BRANCH_STR, &index);
			else
				status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, 
														UF_ROUTE_NPS_STR, &index);
       
			if (status != ERROR_OK)
			{
				UF_print_syslog( "NPS charx not found for object.\n", FALSE );
				return( UF_err_operation_aborted );
			}
        
			/*
			** Build and write each branch point line.  
			*/
			/*sprintf( pcf_string, COMP_BRANCH1_POINT_FMT, 
					branch_pt1[0], branch_pt1[1], branch_pt1[2], 
					charx_list[index].value.r_value );  */
			sprintf( pcf_string, COMP_BRANCH1_POINT_FMT, 
					branch_pt1_r[0], branch_pt1_r[1], branch_pt1_r[2], 
					charx_list[index].value.r_value );  
			status = write_string( pcf_string, pcf_stream );
        
			if (branch_pt2_assigned )
			{
				/*sprintf( pcf_string, COMP_BRANCH2_POINT_FMT, 
						branch_pt2[0], branch_pt2[1], branch_pt2[2], 
						charx_list[index].value.r_value );   */ 
				sprintf( pcf_string, COMP_BRANCH2_POINT_FMT, 
						branch_pt2_r[0], branch_pt2_r[1], branch_pt2_r[2], 
						charx_list[index].value.r_value ); 
				status = write_string( pcf_string, pcf_stream );
			}
          
			/* If the component is a lateral butt weld ( ISOGEN will treat it as TEE)
				then print center point and angle information */
			if ( lateral_butt_weld )
			{
				/*sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
						butt_center[0], butt_center[1], butt_center[2] );*/
				sprintf ( pcf_string, COMP_CENTER_POINT_FMT, 
						butt_center_r[0], butt_center_r[1], butt_center_r[2] );
				write_string ( pcf_string, pcf_stream );
				if ( tee_angle > 91.00 )
					tee_angle = 180.0-tee_angle;
				sprintf ( pcf_string, COMP_ANGLE_FMT, tee_angle );
				write_string ( pcf_string, pcf_stream );
			}
       
			UF_free(pcf_string);
		}
	}
	return (status);
}

/**********************************************************************
* Function Name: copy_position
* 
* Function Description:
*
* Input:
*       input position
* Output
*       output position
* 
* Returns:
*
***********************************************************************/
void pcf_generation::copy_position( double input_pos[3],
                           double output_pos[3])
{
    output_pos[0] = input_pos[0];
    output_pos[1] = input_pos[1];
    output_pos[2] = input_pos[2];
}

//*************Add by CJ******************

bool pcf_generation::check_same_point (double point_1[3], double point_2[3])
{
	if(fabs(point_1[0] - point_2[0]) < 0.001 && fabs(point_1[1] - point_2[1]) < 0.001 && fabs(point_1[2] - point_2[2]) < 0.001)
	{
		return true;
	}else{
		return false;
	}
}

void pcf_generation::coordinate_transform (double input[3], double output[3])
{
	tag_t wcs;
	tag_t matrix_tag;
	double wcs_origin[3];
	double matrix[9];
	UF_CSYS_ask_wcs(&wcs);
	UF_CSYS_ask_csys_info(wcs,&matrix_tag,wcs_origin);
	UF_CSYS_ask_matrix_values(matrix_tag,matrix);
	output[0] = (input[0] - wcs_origin[0])*matrix[0] + (input[1] - wcs_origin[1])*matrix[1] + (input[2] - wcs_origin[2])*matrix[2];
	output[1] = (input[0] - wcs_origin[0])*matrix[3] + (input[1] - wcs_origin[1])*matrix[4] + (input[2] - wcs_origin[2])*matrix[5];
	output[2] = (input[0] - wcs_origin[0])*matrix[6] + (input[1] - wcs_origin[1])*matrix[7] + (input[2] - wcs_origin[2])*matrix[8];
}

double pcf_generation::line_length (double point_1[3], double point_2[3])
{
	return sqrt((point_1[0]-point_2[0])*(point_1[0]-point_2[0]) + (point_1[1]-point_2[1])*(point_1[1]-point_2[1]) + (point_1[2]-point_2[2])*(point_1[2]-point_2[2]));
}

void pcf_generation::point_direction (double start_point[3], double end_point[3], double direction[3])
{
	direction[0] = end_point[0] - start_point[0];
	direction[1] = end_point[1] - start_point[1];
	direction[2] = end_point[2] - start_point[2];
}

void pcf_generation::getComponents(std::vector<Assemblies::Component*>& components)
{
	std::vector< NXOpen::DisplayableObject*> display_obj = displayPart->ModelingViews()->WorkView()->AskVisibleObjects();
	for(std::vector<DisplayableObject*>::iterator it = display_obj.begin(); it != display_obj.end(); it++)
	{
		Assemblies::Component* component = dynamic_cast<Assemblies::Component*>(*it);
		if(component != NULL)
		{
			if(component->GetChildren().size() == 0)
			{
				components.push_back(component);
			}
		}
	}
}

void pcf_generation::get_stub_end_point(tag_t comp_tag)
{
    int     num_ports   = 0;
    int     status      = 0;
    double  endpoint_1[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2[3]   = {0.0, 0.0, 0.0};
	double  endpoint_1_abs[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2_abs[3]   = {0.0, 0.0, 0.0};
	double  vector1[3] = {0.0, 0.0, 0.0};
	double  vector2[3] = {0.0, 0.0, 0.0};
	double  vector3[3] = {0.0, 0.0, 0.0};
	double  length = 0.0;
	double  indiameter = 0.0;
	double  outdiameter = 0.0;
	double  point_1[3]   = {0.0, 0.0, 0.0};
    double  point_2[3]   = {0.0, 0.0, 0.0};
	double  point_3[3]   = {0.0, 0.0, 0.0};
	double	distance_1 = 0.0;
	double	distance_2 = 0.0;
	double	distance_3 = 0.0;
	double	dot = 0.0;
	char message[ MAX_LINE_BUFSIZE ]="";
    tag_t   *ports      = NULL;
    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
	UF_ATTR_info_t info;
	logical has_attr = false;

	UF_ATTR_info_t name_info;
	logical has_name_attr = false;
	logical is_X0002 = false;

	UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_any , UF_ATTR_NOT_ARRAY, &name_info, &has_name_attr);
	if(has_name_attr)
	{
		if(strstr(name_info.string_value, "X0002") != NULL)
		{
			is_X0002 = true;
		}
	}
	UF_ATTR_free_user_attribute_info_strings(&name_info);

    status = UF_ROUTE_ask_object_port (comp_tag, &num_ports, &ports);
	CHECK_STATUS
	status =  UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "STUB_LEN", UF_ATTR_real, UF_ATTR_NOT_ARRAY, &info, &has_attr);
	CHECK_STATUS
	if(has_attr)
	{
		length = info.real_value;
	}

	switch (num_ports)
	{
		case 1:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs );
			CHECK_STATUS   
			UF_ROUTE_ask_port_align_vector(ports[0], vector1);
			
			endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			break;
		case 2:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs); 
			CHECK_STATUS
			status = UF_ROUTE_ask_port_position( ports[1], endpoint_2_abs);
			CHECK_STATUS
			break;
		case 3:
			if(has_attr)
			{
				UF_ROUTE_ask_port_align_vector(ports[0], vector1);
				UF_ROUTE_ask_port_align_vector(ports[1], vector2);
				UF_ROUTE_ask_port_align_vector(ports[2], vector3);
				UF_VEC3_dot ( vector1, vector2, &dot );
				if ( dot > 0 )
				{
					UF_ROUTE_ask_port_position(ports[2], endpoint_1_abs);
					endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector3[0] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
					endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector3[1] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
					endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector3[2] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
				}
				else
				{
					UF_VEC3_dot ( vector1, vector3, &dot );
					if ( dot > 0 ) 
					{
						UF_ROUTE_ask_port_position(ports[1], endpoint_1_abs);
						endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector2[0] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
						endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector2[1] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
						endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector2[2] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
					}
					else 
					{
						UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
						endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
						endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
						endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);	
					}
				}
			}else
			{
				status = UF_ROUTE_ask_port_position( ports[0], point_1 );
				status = UF_ROUTE_ask_port_position( ports[1], point_2 );
				status = UF_ROUTE_ask_port_position( ports[2], point_3 );
				distance_1 = line_length(point_1, point_2);
				distance_2 = line_length(point_1, point_3);
				distance_3 = line_length(point_2, point_3);
				if(distance_1 > distance_2 && distance_1 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[1], endpoint_2_abs);
				}
				if(distance_2 > distance_1 && distance_2 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
				if(distance_3 > distance_1 && distance_3 > distance_2)
				{
					UF_ROUTE_ask_port_position(ports[1], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
			}
			break;
		default:
			sprintf( message, "Invalid number of ports for component %u\n", comp_tag );
			UF_print_syslog( message, FALSE );
			status = UF_err_operation_aborted;
			break;
	}
	coordinate_transform(endpoint_1_abs, endpoint_1);
	coordinate_transform(endpoint_2_abs, endpoint_2);

	indiameter  = determine_in_diameter  ( comp_tag );
    outdiameter = determine_out_diameter ( comp_tag, indiameter );

	pointinfo stubend_endpoint;
	stubend_endpoint.end_point[0] = endpoint_1[0];
	stubend_endpoint.end_point[1] = endpoint_1[1];
	stubend_endpoint.end_point[2] = endpoint_1[2];
	stubend_endpoint.diameter = indiameter;
	stubend_point.push_back(stubend_endpoint);
	stubend_endpoint.end_point[0] = endpoint_2[0];
	stubend_endpoint.end_point[1] = endpoint_2[1];
	stubend_endpoint.end_point[2] = endpoint_2[2];
	stubend_endpoint.diameter = outdiameter;
	stubend_point.push_back(stubend_endpoint);

	UF_ATTR_free_user_attribute_info_strings(&info);
	UF_free( ports );
}
void pcf_generation::write_stub_end_point(tag_t comp_tag, FILE * pcf_stream )
{
    int     num_ports   = 0;
    int     status      = 0;
    double  endpoint_1[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2[3]   = {0.0, 0.0, 0.0};
	double  endpoint_1_abs[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2_abs[3]   = {0.0, 0.0, 0.0};
	double  vector1[3] = {0.0, 0.0, 0.0};
	double  vector2[3] = {0.0, 0.0, 0.0};
	double  vector3[3] = {0.0, 0.0, 0.0};
	double  length = 0.0;
	double  indiameter = 0.0;
	double  outdiameter = 0.0;
	double  point_1[3]   = {0.0, 0.0, 0.0};
    double  point_2[3]   = {0.0, 0.0, 0.0};
	double  point_3[3]   = {0.0, 0.0, 0.0};
	double	distance_1 = 0.0;
	double	distance_2 = 0.0;
	double	distance_3 = 0.0;
	double	dot = 0.0;
	char message[ MAX_LINE_BUFSIZE ]="";
    tag_t   *ports      = NULL;
    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
	UF_ATTR_info_t info;
	logical has_attr = false;

	UF_ATTR_info_t name_info;
	logical has_name_attr = false;
	logical is_X0002 = false;

	UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_any , UF_ATTR_NOT_ARRAY, &name_info, &has_name_attr);
	if(has_name_attr)
	{
		if(strstr(name_info.string_value, "X0002") != NULL)
		{
			is_X0002 = true;
		}
	}
	UF_ATTR_free_user_attribute_info_strings(&name_info);

    status = UF_ROUTE_ask_object_port (comp_tag, &num_ports, &ports);
	CHECK_STATUS
	status =  UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "STUB_LEN", UF_ATTR_real, UF_ATTR_NOT_ARRAY, &info, &has_attr);
	CHECK_STATUS
	if(has_attr)
	{
		length = info.real_value;
	}

	switch (num_ports)
	{
		case 1:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs );
			CHECK_STATUS   
			UF_ROUTE_ask_port_align_vector(ports[0], vector1);
			
			endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			break;
		case 2:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs); 
			CHECK_STATUS
			status = UF_ROUTE_ask_port_position( ports[1], endpoint_2_abs);
			CHECK_STATUS
			break;
		case 3:
			if(has_attr)
			{
				UF_ROUTE_ask_port_align_vector(ports[0], vector1);
				UF_ROUTE_ask_port_align_vector(ports[1], vector2);
				UF_ROUTE_ask_port_align_vector(ports[2], vector3);
				UF_VEC3_dot ( vector1, vector2, &dot );
				if ( dot > 0 )
				{
					UF_ROUTE_ask_port_position(ports[2], endpoint_1_abs);
					endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector3[0] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
					endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector3[1] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
					endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector3[2] / sqrt(vector3[0] * vector3[0] + vector3[1] * vector3[1] + vector3[2] * vector3[2]);
				}
				else
				{
					UF_VEC3_dot ( vector1, vector3, &dot );
					if ( dot > 0 ) 
					{
						UF_ROUTE_ask_port_position(ports[1], endpoint_1_abs);
						endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector2[0] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
						endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector2[1] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
						endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector2[2] / sqrt(vector2[0] * vector2[0] + vector2[1] * vector2[1] + vector2[2] * vector2[2]);
					}
					else 
					{
						UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
						endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
						endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
						endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);	
					}
				}
			}else
			{
				status = UF_ROUTE_ask_port_position( ports[0], point_1 );
				status = UF_ROUTE_ask_port_position( ports[1], point_2 );
				status = UF_ROUTE_ask_port_position( ports[2], point_3 );
				distance_1 = line_length(point_1, point_2);
				distance_2 = line_length(point_1, point_3);
				distance_3 = line_length(point_2, point_3);
				if(distance_1 > distance_2 && distance_1 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[1], endpoint_2_abs);
				}
				if(distance_2 > distance_1 && distance_2 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
				if(distance_3 > distance_1 && distance_3 > distance_2)
				{
					UF_ROUTE_ask_port_position(ports[1], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
			}
			break;
		default:
			sprintf( message, "Invalid number of ports for component %u\n", comp_tag );
			UF_print_syslog( message, FALSE );
			status = UF_err_operation_aborted;
			break;
	}
	coordinate_transform(endpoint_1_abs, endpoint_1);
	coordinate_transform(endpoint_2_abs, endpoint_2);

	indiameter  = determine_in_diameter  ( comp_tag );
    outdiameter = determine_out_diameter ( comp_tag, indiameter );

	write_end_points( endpoint_1, endpoint_2, indiameter, outdiameter, pcf_stream );

	if(is_X0002)
	{
		pointinfo point_temp;
		point_temp.end_point[0] = endpoint_1[0];
		point_temp.end_point[1] = endpoint_1[1];
		point_temp.end_point[2] = endpoint_1[2];
		point_temp.diameter = indiameter;
		control_point.push_back(point_temp);
		point_temp.end_point[0] = endpoint_2[0];
		point_temp.end_point[1] = endpoint_2[1];
		point_temp.end_point[2] = endpoint_2[2];
		point_temp.diameter = outdiameter;
		control_point.push_back(point_temp);
	}
	UF_ATTR_free_user_attribute_info_strings(&info);
	UF_free( ports );
}

void pcf_generation::write_flange_blind_point(tag_t part_tag, tag_t comp_tag, FILE * pcf_stream )
{
    int     num_ports   = 0;
    int     status      = 0;
    double  endpoint_1[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2[3]   = {0.0, 0.0, 0.0};
	double  endpoint_1_abs[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2_abs[3]   = {0.0, 0.0, 0.0};
	double  vector1[3] = {0.0, 0.0, 0.0};
	double  length = 0.0;
	double  indiameter = 0.0;
	double  outdiameter = 0.0;
	char message[ MAX_LINE_BUFSIZE ]="";
    tag_t   *ports      = NULL;
	tag_t	*anchor_tags = NULL;
    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
	UF_ATTR_info_t info;
	logical has_attr = false;

	UF_ATTR_info_t name_info;
	logical has_name_attr = false;
	logical is_X0002 = false;

    status = UF_ROUTE_ask_object_port (comp_tag, &num_ports, &ports);
	CHECK_STATUS

	switch (num_ports)
	{
		case 1:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs );
			CHECK_STATUS   
			UF_ROUTE_ask_port_align_vector(ports[0], vector1);
			status =  UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "FLG_THK", UF_ATTR_real, UF_ATTR_NOT_ARRAY, &info, &has_attr);
			if(has_attr)
			{
				length = info.real_value;
			}
			endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
			break;
		case 2:
			status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs); 
			CHECK_STATUS
			status = UF_ROUTE_ask_port_position( ports[1], endpoint_2_abs);
			CHECK_STATUS
			break;
		default:
			sprintf( message, "Invalid number of ports for component %u\n", comp_tag );
			UF_print_syslog( message, FALSE );
			status = UF_err_operation_aborted;
			break;
	}

	coordinate_transform(endpoint_1_abs, endpoint_1);
	coordinate_transform(endpoint_2_abs, endpoint_2);

	indiameter  = determine_in_diameter  ( comp_tag );
    outdiameter = determine_out_diameter ( comp_tag, indiameter );

	write_end_points( endpoint_1, endpoint_2, indiameter, outdiameter, pcf_stream );

	UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_any , UF_ATTR_NOT_ARRAY, &name_info, &has_name_attr);
	if(has_name_attr)
	{
		if(strstr(name_info.string_value, "X0002") != NULL)
		{
			is_X0002 = true;
		}
	}
	if(is_X0002)
	{pointinfo point_temp;
		point_temp.end_point[0] = endpoint_1[0];
		point_temp.end_point[1] = endpoint_1[1];
		point_temp.end_point[2] = endpoint_1[2];
		point_temp.diameter = indiameter;
		control_point.push_back(point_temp);
		point_temp.end_point[0] = endpoint_2[0];
		point_temp.end_point[1] = endpoint_2[1];
		point_temp.end_point[2] = endpoint_2[2];
		point_temp.diameter = outdiameter;
		control_point.push_back(point_temp);
		
	}
	UF_ATTR_free_user_attribute_info_strings(&info);
	UF_ATTR_free_user_attribute_info_strings(&name_info);
	UF_free( ports );
}

void pcf_generation::write_flange_point(tag_t part_tag, tag_t comp_tag, FILE * pcf_stream )
{
	int     num_ports   = 0;
    int     status      = 0;
    double  endpoint_1[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2[3]   = {0.0, 0.0, 0.0};
	double  endpoint_1_abs[3]   = {0.0, 0.0, 0.0};
    double  endpoint_2_abs[3]   = {0.0, 0.0, 0.0};
	double  vector1[3] = {0.0, 0.0, 0.0};
	double  vector2[3] = {0.0, 0.0, 0.0};
	double  vector3[3] = {0.0, 0.0, 0.0};
	double  length = 0.0;
	double  indiameter = 0.0;
	double  outdiameter = 0.0;
	double  point_1[3]   = {0.0, 0.0, 0.0};
    double  point_2[3]   = {0.0, 0.0, 0.0};
	double  point_3[3]   = {0.0, 0.0, 0.0};
	double	distance_1 = 0.0;
	double	distance_2 = 0.0;
	double	distance_3 = 0.0;
	double	dot = 0.0;
	char message[ MAX_LINE_BUFSIZE ]="";
    tag_t   *ports      = NULL;
	tag_t	*anchor_tags = NULL;
    char    pcf_string[ MAX_LINE_BUFSIZE ] = "";
	UF_ATTR_info_t info;
	UF_ATTR_info_t member_name;
	logical has_attr = false;
	logical has_mem = false;

    status = UF_ROUTE_ask_object_port (comp_tag, &num_ports, &ports);
	CHECK_STATUS

	status =  UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "FLG_THK", UF_ATTR_real, UF_ATTR_NOT_ARRAY, &info, &has_attr);
	status =  UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_string, UF_ATTR_NOT_ARRAY, &member_name, &has_mem);
	CHECK_STATUS
	
	char *X06FB = strstr(member_name.string_value, "X06FB");
	
	if(X06FB != NULL)
	{
		status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs );
		status = UF_ROUTE_ask_port_position( ports[0], endpoint_2_abs );
	}else
	{

		switch (num_ports)
		{
			case 1:
				status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs );
				CHECK_STATUS   
				UF_ROUTE_ask_port_align_vector(ports[0], vector1);
				if(has_attr)
				{
					length = info.real_value;
				}
				endpoint_2_abs[0] = endpoint_1_abs[0] - length * vector1[0] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
				endpoint_2_abs[1] = endpoint_1_abs[1] - length * vector1[1] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
				endpoint_2_abs[2] = endpoint_1_abs[2] - length * vector1[2] / sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
				break;
			case 2:
				status = UF_ROUTE_ask_port_position( ports[0], endpoint_1_abs); 
				CHECK_STATUS
				status = UF_ROUTE_ask_port_position( ports[1], endpoint_2_abs);
				CHECK_STATUS
				break;
			case 3:
				status = UF_ROUTE_ask_port_position( ports[0], point_1 );
				status = UF_ROUTE_ask_port_position( ports[1], point_2 );
				status = UF_ROUTE_ask_port_position( ports[2], point_3 );
				distance_1 = line_length(point_1, point_2);
				distance_2 = line_length(point_1, point_3);
				distance_3 = line_length(point_2, point_3);
				if(distance_1 > distance_2 && distance_1 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[1], endpoint_2_abs);
				}
				if(distance_2 > distance_1 && distance_2 > distance_3)
				{
					UF_ROUTE_ask_port_position(ports[0], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
				if(distance_3 > distance_1 && distance_3 > distance_2)
				{
					UF_ROUTE_ask_port_position(ports[1], endpoint_1_abs);
					UF_ROUTE_ask_port_position(ports[2], endpoint_2_abs);
				}
			default:
				sprintf( message, "Invalid number of ports for component %u\n", comp_tag );
				UF_print_syslog( message, FALSE );
				status = UF_err_operation_aborted;
				break;
		}
	}

	coordinate_transform(endpoint_1_abs, endpoint_1);
	coordinate_transform(endpoint_2_abs, endpoint_2);

	if(X06FB != NULL)
	{
		int pos;
		double dist;
		for(int i = 0; i < stubend_point.size(); i ++)
		{
			if(i == 0)
			{
				pos = i;
				dist = line_length(endpoint_1,stubend_point[i].end_point);
			}
			if(line_length(endpoint_1,stubend_point[i].end_point) < dist)
			{
				pos = i;
				dist = line_length(endpoint_1,stubend_point[i].end_point);
			}
		}	
		if(stubend_point.size() > 0)
		{
			endpoint_1[0] =  stubend_point[pos].end_point[0];
			endpoint_1[1] =  stubend_point[pos].end_point[1];
			endpoint_1[2] =  stubend_point[pos].end_point[2];
			endpoint_2[0] =  stubend_point[pos].end_point[0];
			endpoint_2[1] =  stubend_point[pos].end_point[1];
			endpoint_2[2] =  stubend_point[pos].end_point[2];
		}
	}

	indiameter  = determine_in_diameter  ( comp_tag );
    outdiameter = determine_out_diameter ( comp_tag, indiameter );

	write_end_points( endpoint_1, endpoint_2, indiameter, outdiameter, pcf_stream );

	UF_ATTR_free_user_attribute_info_strings(&info);
	UF_free( ports );
}

void pcf_generation::write_elbow_point(tag_t part_tag, tag_t comp_tag,  FILE * pcf_stream )
{
	int status = ERROR_OK;
	char pcf_string[MAX_LINE_BUFSIZE];

	double end_pt1[3] = {0};
	double end_pt2[3] = {0};
	double end_pt1_r[3] = {0};
	double end_pt2_r[3] = {0};

	double vec1[3] = {0};
	double vec2[3] = {0};
	double center_point[3] = {0};
	double corner_point[3] = {0};
	double pos[3];
	double angle = 0;

	int num_anchors = 0;
	tag_t *anchor_tags = NULL;

	double dist = 0.0;

	double indiameter = 0.0;
	double outdiameter = 0.0;

	tag_t *port_tags = NULL;
	int num_ports = 0;
	
	tag_t start_port = NULL_TAG;
	tag_t end_port = NULL_TAG;

	bool is_cut_elbow = false;

	UF_ROUTE_charx_p_t charx_list;
	int charx_count;

	status = UF_ROUTE_ask_characteristics( comp_tag,  UF_ROUTE_CHARX_TYPE_ANY,  &charx_count,  &charx_list);
   
	/*
	** Find the component ports and use them as the end points and
	** branch points. 
	*/
	
	UF_ROUTE_ask_object_port( comp_tag, &num_ports, &port_tags );
	start_port = port_tags[0];
	end_port = port_tags[1];
	find_start_and_end_port (port_tags, &start_port, &end_port );
	
	status = UF_ROUTE_ask_port_position( start_port, end_pt1); 
	CHECK_STATUS
	status = UF_ROUTE_ask_port_position( end_port, end_pt2);
	CHECK_STATUS

	status = UF_ROUTE_ask_port_align_vector(start_port, vec1);
	CHECK_STATUS
	status = UF_ROUTE_ask_port_align_vector(end_port, vec2);
	CHECK_STATUS

	coordinate_transform(end_pt1, end_pt1_r);
	coordinate_transform(end_pt2, end_pt2_r);

	ask_comp_anchors( part_tag, comp_tag, &num_anchors, &anchor_tags);
	if (num_anchors >= 1)
	{
		status = UF_ROUTE_ask_anchor_position( anchor_tags[0], pos);  
		CHECK_STATUS
		coordinate_transform(pos, corner_point);
	}

	center_point[0] = end_pt1_r[0] + end_pt2_r[0] - corner_point[0];
	center_point[1] = end_pt1_r[1] + end_pt2_r[1] - corner_point[1];
	center_point[2] = end_pt1_r[2] + end_pt2_r[2] - corner_point[2];

	indiameter  = determine_in_diameter  ( comp_tag );
    outdiameter = determine_out_diameter ( comp_tag, indiameter );

	dist = line_length(end_pt1_r, end_pt2_r);
	
	bool end_1 = false;
	bool end_2 = false;
	for(int i = 0; i < pipe_endpoint.size(); i++ )
	{
		if(check_same_point(pipe_endpoint[i].end_point, end_pt1_r))
		{
			end_1 = true;
		}
		if(check_same_point(pipe_endpoint[i].end_point, end_pt2_r))
		{
			end_2 = true;
		}

	}
	
	if(cut_elbow_label.size()>0)
	{	
		if(end_1 && !end_2)
		{
			for(int i = 0; i < cut_elbow_label.size(); i++ )
			{
				if(line_length(cut_elbow_label[i].end_point, end_pt2_r) < dist)
				{
					end_pt2_r[0] = cut_elbow_label[i].end_point[0];
					end_pt2_r[1] = cut_elbow_label[i].end_point[1];
					end_pt2_r[2] = cut_elbow_label[i].end_point[2];
					is_cut_elbow = true;
				}
			}
		}

		if(!end_1 && end_2)
		{
			for(int i = 0; i < cut_elbow_label.size(); i++ )
			{
				if(line_length(cut_elbow_label[i].end_point, end_pt1_r) < dist)
				{
					end_pt1_r[0] = cut_elbow_label[i].end_point[0];
					end_pt1_r[1] = cut_elbow_label[i].end_point[1];
					end_pt1_r[2] = cut_elbow_label[i].end_point[2];
					is_cut_elbow = true;
				}
			}
		}
	}

	write_end_points( end_pt1_r, end_pt2_r, indiameter, outdiameter, pcf_stream );

	if(is_cut_elbow)
	{
		double n1[3];
		double n2[3];
		n1[0] = end_pt1_r[0] - center_point[0];
		n1[1] = end_pt1_r[1] - center_point[1];
		n1[2] = end_pt1_r[2] - center_point[2];
		n2[0] = end_pt2_r[0] - center_point[0];
		n2[1] = end_pt2_r[1] - center_point[1];
		n2[2] = end_pt2_r[2] - center_point[2];
		angle = 57.29578 * acos((n1[0]*n2[0]+n1[1]*n2[1]+n1[2]*n2[2])/(sqrt(n1[0]*n1[0]+n1[1]*n1[1]+n1[2]*n1[2])*sqrt(n2[0]*n2[0]+n2[1]*n2[1]+n2[2]*n2[2])));
		
		write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );     
		sprintf(pcf_string, "    ANGLE %.2f\n", angle);
		write_string(pcf_string, pcf_stream);


	}else
	{
		write_center_point( part_tag, comp_tag, charx_count, charx_list, pcf_stream );      
		write_angle( charx_count, charx_list, pcf_stream );
	}

	UF_free( port_tags );

}

void pcf_generation::write_comp_spool_identifier(tag_t comp_tag,  FILE * pcf_stream )
{
	int status = 0;
	char pcf_string[MAX_LINE_BUFSIZE];
	UF_ATTR_info_t info;
	UF_ATTR_info_t info_name;
	logical has_attr = false;

	UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "KIT", UF_ATTR_string, UF_ATTR_NOT_ARRAY, &info, &has_attr);
	if(has_attr){
		sprintf(pcf_string,"    SPOOL-IDENTIFIER  %s\n", info.string_value);
		write_string(pcf_string, pcf_stream);
	}else{
		UF_ATTR_get_user_attribute_with_title_and_type(comp_tag, "MEMBER_NAME", UF_ATTR_string, UF_ATTR_NOT_ARRAY, &info_name, &has_attr);
		if(has_attr){
			sprintf(pcf_string,"%s has no SPOOL-IDENTIFIER", info_name.string_value);
			uc1601(pcf_string,1);
		}
	}

}

//void pcf_generation::write_stock_spool_identifier( int charx_count, UF_ROUTE_charx_p_t charx_list, FILE * pcf_stream )
//{ 
//	int status = 0;
//	int index = 0; 
//	char pcf_string[MAX_LINE_BUFSIZE];
//
//	status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, "KIT", &index );
//	if (status != ERROR_OK && index > -1)
//	{
//		sprintf(pcf_string,"SPOOL-IDENTIFIER  %s", charx_list[index].value.s_value);
//		write_string(pcf_string, pcf_stream);
//	}else
//	{
//		status = UF_ROUTE_find_title_in_charx( charx_count, charx_list, "MEMBER_NAME", &index );
//		if (status != ERROR_OK && index > -1)
//		{
//			sprintf(pcf_string,"%s has no SPOOL-IDENTIFIER", charx_list[index].value.s_value);
//			uc1601(pcf_string,1);
//		}
//	}
//
//}