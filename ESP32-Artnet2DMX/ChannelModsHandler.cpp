
#include "ChannelModsHandler.h"

ChannelModsHandler::ChannelModsHandler() {
}

ChannelModsHandler::~ChannelModsHandler() {
  this->Clear();
}

void ChannelModsHandler::Clear() {
  m_channel_mods_vector.clear();
}

void ChannelModsHandler::AddMod( const ChannelMod& mod ) {
  m_channel_mods_vector.push_back( mod );

  this->SortBySequenceAndRenumber();
}

void ChannelModsHandler::AddMod( const unsigned int channel_number, const unsigned int mod_type, const unsigned int mod_value ) {
  ChannelMod mod;
  mod.m_sequence  = this->GetNextSequenceForChannel( channel_number );
  mod.m_channel   = channel_number;
  mod.m_mod_type  = mod_type;
  mod.m_mod_value = mod_value;

  m_channel_mods_vector.push_back( mod );

  this->SortBySequenceAndRenumber();
}

void ChannelModsHandler::RemoveMod( const unsigned int sequence_number ) {
  for( auto mods_itr = m_channel_mods_vector.begin(); mods_itr != m_channel_mods_vector.end(); ) {
    if( mods_itr->m_sequence == sequence_number ) {
      m_channel_mods_vector.erase( mods_itr );
      break;
    }
    ++mods_itr;
  }
  this->SortBySequenceAndRenumber();
}

void ChannelModsHandler::UpdateForModType( const unsigned int sequence_number, const unsigned int mod_type ) {
  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence == sequence_number ) {
      mod.m_mod_type = mod_type;
      break;
    }
  }
}

void ChannelModsHandler::UpdateForModValue( const unsigned int sequence_number, const unsigned int mod_value ) {
  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence == sequence_number ) {
      mod.m_mod_value = mod_value;
      break;
    }
  }
}

void ChannelModsHandler::RemoveAllForChannel( const unsigned int channel_number ) {
  for( auto mods_itr = m_channel_mods_vector.begin(); mods_itr != m_channel_mods_vector.end(); ) {
    if( mods_itr->m_channel == channel_number ) {
      m_channel_mods_vector.erase( mods_itr );
      break;
    }
    ++mods_itr;
  }
  this->SortBySequenceAndRenumber();
}

const std::vector< ChannelMod >& ChannelModsHandler::GetModsVector() const {
  return m_channel_mods_vector;
}

unsigned int ChannelModsHandler::GetMaxSequence() {
  unsigned int sequence_max = 0;

  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_sequence > sequence_max ) {
      sequence_max = mod.m_sequence;
    }
  }

  return sequence_max;
}

unsigned int ChannelModsHandler::GetNextSequenceForChannel( const unsigned int channel_number ) {
  unsigned int sequence_max = 0;
  unsigned int sequence_no_other = 1;

  for( auto& mod: m_channel_mods_vector ) {
    if( mod.m_channel == channel_number && mod.m_sequence > sequence_max ) {
      sequence_max = mod.m_sequence;
    } else if( mod.m_channel < channel_number ) {
      sequence_no_other = mod.m_sequence;
    }
  }

  if( sequence_max == 0 ) {
    return sequence_no_other + 1;
  }
  return sequence_max + 1;
}

void ChannelModsHandler::SortBySequenceAndRenumber() {
  // Sort
  std::sort( m_channel_mods_vector.begin(), m_channel_mods_vector.end(), ChannelMod::CompareChannelModSequence );

  // Re-Sequence
  unsigned int sequence_new = 10;
  for( auto& mod: m_channel_mods_vector ) {
    mod.m_sequence = sequence_new;
    sequence_new += 10;
  }
}
