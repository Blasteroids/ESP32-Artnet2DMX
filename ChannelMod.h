#ifndef _CHANNELMOD_H_
#define _CHANNELMOD_H_

struct ChannelMod {
  unsigned int m_sequence;
  unsigned int m_channel;
  unsigned int m_mod_type;
  unsigned int m_mod_value;
    
  static bool CompareChannelModSequence( const ChannelMod& mod1, const ChannelMod& mod2 ) {
    return mod1.m_sequence < mod2.m_sequence;
  }

};

enum CHANNELMODTYPE : int {
  NOTHING            = 0,
  EQUALS_VALUE       = 1,
  ADD_VALUE          = 2,
  MINUS_VALUE        = 3,
  COPY_FROM_CHANNEL  = 4,
  ADD_FROM_CHANNEL   = 5,
  MINUS_FROM_CHANNEL = 6,
  MAX                = 6,
};

inline const char* ModTypeAsString( int modtype ) {
  switch( modtype ) {
    case NOTHING:            return "Select Mod";
    case EQUALS_VALUE:       return "Equals Value";
    case ADD_VALUE:          return "Add Value";
    case MINUS_VALUE:        return "Minus Value";
    case COPY_FROM_CHANNEL:  return "Copy From Channel";
    case ADD_FROM_CHANNEL:   return "Add From Channel";
    case MINUS_FROM_CHANNEL: return "Minus From Channel";
    default:                 return "Unknown Mod Type";
  }
};

#endif
