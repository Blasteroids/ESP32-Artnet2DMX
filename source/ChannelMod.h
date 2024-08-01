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
  NOTHING              = 0,
  EQUALS_VALUE         = 1,
  ADD_VALUE            = 2,
  MINUS_VALUE          = 3,
  COPY_FROM_CHANNEL    = 4,
  ADD_FROM_CHANNEL     = 5,
  MINUS_FROM_CHANNEL   = 6,
  ABOVE_0_ADD_VALUE    = 7,
  ABOVE_0_MINUS_VALUE  = 8,
  COPY_FROM_ARTNET     = 9,
  ADD_FROM_ARTNET      = 10,
  MINUS_FROM_ARTNET    = 11,
  IF_0_ADD_FROM_ARTNET = 12,
  MAX                  = 12,
};

inline const char* ModTypeAsString( int modtype ) {
  switch( modtype ) {
    case NOTHING:              return "Select mod";
    case EQUALS_VALUE:         return "Equals value";
    case ADD_VALUE:            return "Add value";
    case MINUS_VALUE:          return "Minus value";
    case COPY_FROM_CHANNEL:    return "Copy from output channel";
    case ADD_FROM_CHANNEL:     return "Add from output Channel";
    case MINUS_FROM_CHANNEL:   return "Minus from output channel";
    case ABOVE_0_ADD_VALUE:    return "Only add if value above 0";
    case ABOVE_0_MINUS_VALUE:  return "Only minus if value above 0";
    case COPY_FROM_ARTNET:     return "Copy from Art-Net channel";
    case ADD_FROM_ARTNET:      return "Add from Art-Net channel";
    case MINUS_FROM_ARTNET:    return "Minus from Art-Net channel";
    case IF_0_ADD_FROM_ARTNET: return "If 0, add from Art-Net channel";
    default:                   return "Unknown Mod Type";
  }
};

#endif
