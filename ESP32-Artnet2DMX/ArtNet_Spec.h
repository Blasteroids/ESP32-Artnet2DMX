#ifndef _ARTNET_SPEC_H_
#define _ARTNET_SPEC_H_

// Refer to https://art-net.org.uk/ for full specification.

// Included here is only some very basic items.

// This header only includes the following basic (not full) items;
//    Art-Net Packet Header  (First 10 bytes)
//    Art-Net Packet DMX (Standard dmx packet structure)
//    Art-Net Packet Poll
//    Art-Net Packet Poll Reply

#define ARTNET_HEADER_ID        "Art-Net"
#define ARTNET_VERSION          14
#define ARTNET_UDP_PORT         6454
#define ARTNET_OPCODE_POLL      0x2000
#define ARTNET_OPCODE_POLLREPLY 0x2100
#define ARTNET_OPCODE_DMX       0x5000

#define ARTNET_PACKET_MINSIZE_HEADER    10
#define ARTNET_PACKET_MINSIZE_DMX       21
#define ARTNET_PACKET_MINSIZE_POLL      14
#define ARTNET_PACKET_MINSIZE_POLLREPLY 207
#define ARTNET_PACKET_MAXSIZE           530   // DMX = 10 for header + 8 packet info + 512 dmx data. To Check: Any other packets go larger?
#define ARTNET_PACKET_PAYLOAD_START     10

#pragma pack( push, 1 ) // Set packing alignment to 1 byte

typedef struct ArtNetPacketHeader
{
  uint8_t  m_ID[ 8 ];           //  1: "Art-Net" case sensitive.
  uint16_t m_OpCode;            //  2: The opcode that specifies the remainder of the packet data.  Low byte first.
} __attribute__( ( packed ) ) ArtNetPacketHeader;

typedef struct ArtNetPacketDMX
{
  uint8_t m_ProtocolHi;         //  3: High byte of the Art-Net protocol revision number.
  uint8_t m_ProtocolLo;         //  4: Low byte of the Art-Net protocol revision number.
  uint8_t m_Sequence;           //  5: Monotonic counter.
  uint8_t m_Physical;           //  6: The physical input port from which DMX512 data was input. 
  uint8_t m_SubUni;             //  7: low byte universe.
  uint8_t m_Net;                //  8: high byte universe.
  uint8_t m_LengthHi;           //  9: Data length, must be even number in range 2-512.
  uint8_t m_Length;             // 10: Low byte of above.
  uint8_t m_Data[ 512 ];        // 11: DMX512 data.  Max 512, but determined by length bytes.
} __attribute__( ( packed ) ) ArtNetPacketDMX;

// Consumers of ArtPoll shall accept as valid a packet of length 14 bytes or larger (Page 17)
// Minimum poll is 14 bytes.
typedef struct ArtNetPacketPoll
{
  uint8_t m_ProtocolHi;         //  3: High byte of the Art-Net protocol revision number.
  uint8_t m_ProtocolLo;         //  4: Low byte of the Art-Net protocol revision number.
  uint8_t m_Flags;              //  5: 0x00 - Disable receiving diagnostics, etc & only send ArtPollReply in response to an ArtPoll.
  uint8_t m_DiagPriority;       //  6: 0x10 Low priority message.
} __attribute__( ( packed ) ) ArtNetPacketPoll;

// Consumers of ArtPollReply shall accept as valid a packet of length 207 bytes or larger.
// Any missing fields are assumed to be zero.
typedef struct ArtNetPacketPollReply
{
  uint8_t  m_IPAddress[ 4 ];    //  3: Array containing nodes IP address.
  uint16_t m_Port;              //  4: Always 0x1936 Lo byte first.
  uint8_t  m_VersInfoH;         //  5: High byte of Node’s firmware revision number.
  uint8_t  m_VersInfoL;         //  6: Low byte of Node’s firmware revision number.
  uint8_t  m_NetSwitch;         //  7:
  uint8_t  m_SubSwitch;         //  8:
  uint8_t  m_OemHi;             //  9:
  uint8_t  m_Oem;               // 10:
  uint8_t  m_Ubea_Version;      // 11:
  uint8_t  m_Status1;           // 12:
  uint8_t  m_EstaManLo;         // 13:
  uint8_t  m_EstaManHi;         // 14:
  uint8_t  m_PortName[ 18 ];    // 15:
  uint8_t  m_LongName[ 64 ];    // 16:
  uint8_t  m_NodeReport[ 64 ];  // 17:
  uint8_t  m_NumPortsHi;        // 18:
  uint8_t  m_NumPortsLo;        // 19:
  uint8_t  m_PortTypes[ 4 ];    // 20:
  uint8_t  m_GoodInput[ 4 ];    // 21:
  uint8_t  m_GoodOutputA[ 4 ];  // 22:
  uint8_t  m_SwIn[ 4 ];         // 23:
  uint8_t  m_SwOut[ 4 ];        // 24:
  uint8_t  m_AcnPriority;       // 25:
  uint8_t  m_SwMacro;           // 26:
  uint8_t  m_SwRemote;          // 27:
  uint8_t  m_Spare_1;           // 28:
  uint8_t  m_Spare_2;           // 29:
  uint8_t  m_Spare_3;           // 30:
  uint8_t  m_Style;             // 31:
  uint8_t  m_MAC_1_Hi;          // 32:
  uint8_t  m_MAC_2;             // 33:
  uint8_t  m_MAC_3;             // 34:
  uint8_t  m_MAC_4;             // 35:
  uint8_t  m_MAC_5;             // 36:
  uint8_t  m_MAC_6_Lo;          // 37:

} __attribute__( ( packed ) ) ArtNetPacketPollReply;

#pragma pack( pop ) // Restore original packing alignment

#endif
