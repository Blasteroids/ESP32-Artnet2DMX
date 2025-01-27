#ifndef _CHANNELMODSHANDLER_H_
#define _CHANNELMODSHANDLER_H_

#include <vector>
#include <algorithm> // std::sort
#include "ChannelMod.h"

class ChannelModsHandler {
public:
  ChannelModsHandler();

  ~ChannelModsHandler();

  void Clear();
  void AddMod( const ChannelMod& mod );
  void AddMod( const unsigned int channel_number, const unsigned int mod_type, const unsigned int mod_value );
  void RemoveMod( const unsigned int sequence_number );
  void UpdateForModType( const unsigned int sequence_number, const unsigned int mod_type );
  void UpdateForModValue( const unsigned int sequence_number, const unsigned int mod_value );
  void RemoveAllForChannel( const unsigned int channel_number );

  const std::vector< ChannelMod >& GetModsVector() const;

private:
  unsigned int GetMaxSequence();
  unsigned int GetNextSequenceForChannel( const unsigned int channel_number );
  void         SortBySequenceAndRenumber(); 

  std::vector< ChannelMod > m_channel_mods_vector;

};


#endif