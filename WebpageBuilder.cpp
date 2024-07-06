#include "WebpageBuilder.h"

WebpageBuilder::WebpageBuilder() {
}

WebpageBuilder::~WebpageBuilder() {
}

void WebpageBuilder::StartPage() {
  m_html = "<!DOCTYPE html><html>";
}

void WebpageBuilder::EndPage() {
  m_html += "</html>";
}

void WebpageBuilder::StartBody() {
  m_html += "<body>";
}

void WebpageBuilder::EndBody() {
  m_html += "</body>";
}

void WebpageBuilder::AddFormAction( const String& action, const String& method ) {
  m_html += "<form action=\"" + action + "\"method=\"" + method + "\">";
}

void WebpageBuilder::EndFormAction() {
  m_html += "</form>";
}

void WebpageBuilder::StartDivClass( const String& class_name ) {
  m_html += "<div class=\"" + class_name + "\">";
}

void WebpageBuilder::EndDiv() {
  m_html += "</div>";
}

void WebpageBuilder::StartCenter() {
  m_html += "<center>";
}
  
void WebpageBuilder::EndCenter() {
  m_html += "</center>";
}

void WebpageBuilder::AddHeading( const String& heading_text ) {
  m_html += "<h1 style=\"font-size:45px;\">" + heading_text + "</h1>";
}

void WebpageBuilder::AddLabel( const String& label_for, const String& label_text ) {
  m_html += "<label for=\"" + label_for + "\">" + label_text + "</label>";
}

void WebpageBuilder::AddInputType( const String& input_type, const String& input_id, const String& input_name, const String& input_value, const String& placeholder, bool required ) {
  m_html += "<input type=\"" + input_type + "\" id=\"" + input_id + "\" name=\"" + input_name + "\"";

  if( input_value.length() > 0 ) {
    m_html += " value=\"" + input_value + "\"";
  }
  
  if( required ) {
    m_html += " required";
  }
  
  if( placeholder.length() > 0 ) {
    m_html += " placeholder=\"" + placeholder + "\"";
  }
  
  m_html += ">";
}

void WebpageBuilder::AddButton( const String& type, const String& display_name ) {
  m_html += "<input type=\"" + type + "\" value=\"" + display_name + "\">";
}

void WebpageBuilder::AddButtonAction( const String& form_action, const String& display_name ) {
  m_html += "<button formaction=\"" + form_action + "\">" + display_name + "</button>";
}

void WebpageBuilder::AddButtonActionForm( const String& form_action, const String& display_name ) {
  m_html += "<form><button formaction=\"" + form_action + "\">" + display_name + "</button></form>";
}

void WebpageBuilder::AddTitle( const String& title ) {
  m_html += "<head><title>" + title + "</title></head>";
}

void WebpageBuilder::AddText( const String& text ) {
  m_html += text;
}

void WebpageBuilder::StartCircleStyle( const String& name ) {
  m_html += "<style>";
  m_html += "." + name + " { width: 90vw; height: 90vw; border-radius: 50%; background-color: #f0f0f0; ";
  m_html += "display: flex; justify-content: center; align-items: center; position: relative; } ";
  m_html += ".circle-button { width: 4vw; height: 4vw; border-radius: 50%; background: transparent; ";
  m_html += "border: 1vw solid; text-align: center; line-height: 4vw; font-size: 2vw; position: absolute; }";
  m_html += ".green {border-color: green;}";
  m_html += ".red {border-color: red;}";
  m_html += ".yellow {border-color: yellow;}";
  m_html += ".blue {border-color: blue;}";
}

void WebpageBuilder::AddCircleButtonStyle( int number, int position_x, int position_y ) {
  m_html += ".circle-button:nth-child(";
  m_html += String( number );
  m_html += "){ transform: translate(";
  m_html += String( position_x );
  m_html += "%,";
  m_html += String( position_y );
  m_html += "%); }";
}

void WebpageBuilder::EndCircleStyle() {
  m_html += "</style>";
}

void WebpageBuilder::AddCircleContainer( int display_number, const char* colour, const String& name ) {
  m_html += "<div class=\"circle-button " + String( colour ) + "\" onclick=\"window.location.href='" + name + "'\">" + String( display_number ) + "</div>";
}

void WebpageBuilder::AddGridStyle( const char* name, int columns ) {
  m_html += "<style>." + String( name ) + " {display: grid; grid-template-columns: repeat(" + String( columns ) + ", 1fr); grid-template-rows: repeat(" + String( columns ) + ", 1fr); gap: 2px;}";
  m_html += "." + String( name ) + " > div {background-color: #f2f2f2; text-align: center; padding: 10px; font-size: 20px;}";
  m_html += ".default {background-color: white; color: black; }</style>";
}

void WebpageBuilder::AddGridCellText( const String& text ) {
  m_html += "<div class=\"default\">" + text + "</div>";
}

void WebpageBuilder::AddGridEntryNumberCell( const String& name, int value, int min, int max, bool required ) {
  m_html += "<input type=\"number\" name='" + name + "' value=\"" + value + "\" min=\"" + String( min ) + "\" max=\"" + String( max ) + "\"";
  if( required ) {
    m_html += " required/>";
  } else {
    m_html += "/>";
  }
}

void WebpageBuilder::AddGridEntryTextCell( const String& name, const String& value, bool required ) {
  m_html += "<input type=\"text\" class=\"form-input\" name='" + name + "' value=\"" + value + "\"";
  if( required ) {
    m_html += " required/>";
  } else {
    m_html += "/>";
  }
}

void WebpageBuilder::AddEnabledSelection( const String& name, const String& id, bool enabled ) {
  m_html += "<select name=\"" + name + "\" id=\"" + id + "\"><option value=\"Enabled\"";
  if( enabled ) {
    m_html += " selected";
  }
  m_html += ">Enabled</option><option value=\"Disabled\"";
  if( !enabled ) {
    m_html += " selected";
  }
  m_html += ">Disabled</option></select>";
}

void WebpageBuilder::AddSpace( int amount ) {
  for( int i = 0; i < amount; i++ )
  {
    m_html += "&nbsp";
  }
}

void WebpageBuilder::AddBreak( int amount ) {
  for( int i = 0; i < amount; i++ )
  {
    m_html += "<br>";
  }
}

void WebpageBuilder::AddStandardViewportScale() {
  m_html += "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
}
