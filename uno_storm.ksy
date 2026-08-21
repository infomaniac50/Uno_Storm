meta:
  id: uno_storm
  endian: le
seq:
  - id: packets
    type: uno_storm_packet
    repeat: eos

types:
  uno_storm_packet_header:
    seq:
      - id: magic
        size: 2
      - id: packet_type
        type: u1
        enum: packet_types
      - id: packet_size
        type: u2be
  uno_storm_packet:
    seq:
      - id: header
        type: uno_storm_packet_header
      - id: payload
        type:
          switch-on: header.packet_type
          cases:
            'packet_types::lightning_packet': lightning_packet
            'packet_types::weather_radio_packet': weather_radio_packet
            'packet_types::same_status_packet': same_status_packet
            'packet_types::same_message_packet': same_message_packet
            'packet_types::boot_message_packet': boot_message_packet
  boot_message_packet:
    seq:
      - id: message
        type: str
        encoding: ascii
        terminator: 0x0a
  lightning_packet:
    seq:
      - id: interrupt_type
        type: u1
      - id: distance
        type: u1
      - id: energy
        type: u4
  weather_radio_packet:
    seq:
      - id: weather_radio_interrupt_type
        type: u1
        enum: weather_radio_interrupt_types
      - id: wb_subpacket_type
        type:
          switch-on: weather_radio_interrupt_type
          cases:
            'weather_radio_interrupt_types::wb_stc_interrupt': seek_tune_complete_packet
            'weather_radio_interrupt_types::wb_rsq_interrupt': received_signal_quality_packet
            'weather_radio_interrupt_types::wb_alert_tone_interrupt': alert_tone_packet
            'weather_radio_interrupt_types::wb_error_interrupt': error_packet
  seek_tune_complete_packet:
    seq:
      - id: rssi
        type: u1
      - id: snr
        type: u1
      - id: frequency
        type: strz
        encoding: ascii
  received_signal_quality_packet:
    seq:
      - id: rssi
        type: u1
      - id: snr
        type: u1
      - id: freqoff
        type: s1
  alert_tone_packet:
    seq:
      - id: alert_tone
        type: u1
  error_packet:
    seq:
      - id: no_value
        size: 0
  same_status_packet:
    seq:
      - id: same_interrupt_type
        type: u1
        enum: same_interrupt_types
  same_message_packet:
    seq:
      - id: same_interrupt_type
        type: u1
        enum: same_interrupt_types
      - id: originator_name_size
        type: u1
      - id: originator_name
        type: str
        encoding: ascii
        size: originator_name_size
      - id: event_name_size
        type: u1
      - id: event_name
        type: str
        encoding: ascii
        size: event_name_size
      - id: call_sign_size
        type: u1
      - id: call_sign
        type: str
        encoding: ascii
        size: call_sign_size
      - id: locations
        type: u1
      - id: location_codes
        type: u4
        repeat: expr
        repeat-expr: locations
      - id: duration
        type: u2
      - id: day
        type: u2
      - id: hour
        type: u1
      - id: minute
        type: u1

enums:
  packet_types:
    0x01: lightning_packet
    0x02: weather_radio_packet
    0x03: same_status_packet
    0x04: same_message_packet
    0x05: boot_message_packet
  weather_radio_interrupt_types:
    0x01: wb_stc_interrupt
    0x02: wb_alert_tone_interrupt
    0x04: wb_same_interrupt
    0x08: wb_rsq_interrupt
    0x40: wb_error_interrupt
    0x80: wb_cts_interrupt
  same_interrupt_types:
    3: wb_same_preamble
    5: wb_same_end_of_message
    7: wb_same_message_received
