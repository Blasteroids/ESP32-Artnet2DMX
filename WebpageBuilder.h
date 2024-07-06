#ifndef _WEBPAGE_BUILDER_H_
#define _WEBPAGE_BUILDER_H_

#include "Arduino.h"

class WebpageBuilder
{
public:
  WebpageBuilder();
  
  ~WebpageBuilder();

  void StartPage();
  
  void EndPage();
  
  void StartBody();
  
  void EndBody();
  
  void AddFormAction( const String& action, const String& method );
 
  void EndFormAction();
  
  void StartDivClass( const String& class_name );

  void EndDiv();

  void StartCenter();
  
  void EndCenter();

  void AddHeading( const String& heading_text );

  void AddLabel( const String& label_for, const String& label_text );

  void AddInputType( const String& input_type, const String& input_id, const String& input_name, const String& input_value, const String& placeholder, bool required );

  void AddButton( const String& type, const String& display_name );
  
  void AddButtonAction( const String& form_action, const String& display_name );

  void AddButtonActionForm( const String& form_action, const String& display_name );

  void AddTitle( const String& title );
  
  void AddText( const String& text );
    
  void StartCircleStyle( const String& name );
  
  void AddCircleButtonStyle( int number, int position_x, int position_y );
  
  void StopCircleStyle();
  
  void EndCircleStyle();

  void AddCircleContainer( int display_number, const char* colour, const String& name );
  
  void AddGridStyle( const char* name, int columns );
  
  void AddGridCellText( const String& text );
  
  void AddGridEntryNumberCell( const String& name, int value, int min, int max, bool required );
  
  void AddGridEntryTextCell( const String& name, const String& value, bool required );
  
  void AddEnabledSelection( const String& name, const String& id, bool enabled );
  
  void AddSpace( int amount );
  
  void AddBreak( int amount );
  
  void AddStandardViewportScale();

  String m_html;

private:

};

#endif
