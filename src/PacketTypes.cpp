#include <stdint.h>
#include <SI4707.h>
#include <PacketTypes.h>

size_t startPacket(UnoStormPacketHeader header)
{
  // Magic here in case the host software restarts mid-packet.
  uint8_t buffer[5] = {0xAA, 0xFF, header.packetType, highByte(header.payloadSize), lowByte(header.payloadSize)};
  return sendBuffer(5, buffer);
}

size_t sendPacket(UnoStormPacket packet)
{
  return startPacket(packet.header) +
         sendBuffer(packet.header.payloadSize, packet.payload);
}

size_t sendBuffer(size_t bufferSize, const uint8_t *buffer)
{
  size_t bytesSent = 0;

  for (size_t i = 0; i < bufferSize; i++)
  {
    bytesSent += Serial.write(buffer[i]);
  }

  return bytesSent;
}
