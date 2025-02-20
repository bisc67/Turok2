//############################################################################
//##                                                                        ##
//##  Copyright (C) RAD Game Tools, Inc.                                    ##
//##                                                                        ##
//##  Contact RAD Game Tools at 425-893-4300 for technical support.         ##
//##                                                                        ##
//############################################################################

// The string codes:
//   d=double word
//   p=pointer (translate to a 16:16 for just this call)
//   t=persistant pointer (translate by looking 12 bytes before ptr for 16:16 ptr)

// The final value is the number of parameters.  It is ORed with 0x80 if
//   the return value of the call needs to be translated from a 16:16 pointer


RAD_process(AIL,mem_alloc_lock,"d",0x81)
RAD_process(AIL,mem_free_lock,"t",1)
RAD_process(AIL,file_error,"",0)
RAD_process(AIL,file_size,"p",1)
RAD_process(AIL,file_read,"pt",0x82)
RAD_process(AIL,file_write,"ptd",3)
RAD_process(AIL,WAV_file_write,"ptddd",5)
RAD_process(AIL,startup,"",0)
RAD_process(AIL,shutdown,"",0)
RAD_process(AIL,set_preference,"dd",2)
RAD_process(AIL,set_error,"p",1)
RAD_process(AIL,last_error,"",0x80)
RAD_process(AIL,delay,"d",1)
RAD_process(AIL,waveOutOpen,"ppdp",4)
RAD_process(AIL,waveOutClose,"d",1)
RAD_process(AIL,digital_handle_release,"d",1)
RAD_process(AIL,digital_handle_reacquire,"d",1)
RAD_process(AIL,digital_CPU_percent,"d",1)
RAD_process(AIL,digital_latency,"d",1)
RAD_process(AIL,serve,"",0)
RAD_process(AIL,allocate_sample_handle,"d",1)
RAD_process(AIL,allocate_file_sample,"dtd",3)
RAD_process(AIL,release_sample_handle,"d",1)
RAD_process(AIL,init_sample,"d",1)
RAD_process(AIL,set_sample_file,"dtd",3)
RAD_process(AIL,set_named_sample_file,"dttdd",5)
RAD_process(AIL,set_sample_processor,"ddd",3)
RAD_process(AIL,set_digital_driver_processor,"ddd",3)
RAD_process(AIL,set_sample_address,"dtd",3)
RAD_process(AIL,set_sample_type,"ddd",3)
RAD_process(AIL,start_sample,"d",1)
RAD_process(AIL,stop_sample,"d",1)
RAD_process(AIL,resume_sample,"d",1)
RAD_process(AIL,end_sample,"d",1)
RAD_process(AIL,set_sample_playback_rate,"dd",2)
RAD_process(AIL,set_sample_volume,"dd",2)
RAD_process(AIL,set_sample_pan,"dd",2)
RAD_process(AIL,set_sample_loop_count,"dd",2)
RAD_process(AIL,set_sample_loop_block,"ddd",3)
RAD_process(AIL,sample_status,"d",1)
RAD_process(AIL,sample_playback_rate,"d",1)
RAD_process(AIL,sample_volume,"d",1)
RAD_process(AIL,sample_pan,"d",1)
RAD_process(AIL,sample_loop_count,"d",1)
RAD_process(AIL,set_digital_master_volume,"dd",2)
RAD_process(AIL,set_sample_reverb,"dddd",4)
RAD_process(AIL,sample_reverb,"dppp",4)
RAD_process(AIL,digital_master_volume,"d",1)
RAD_process(AIL,minimum_sample_buffer_size,"ddd",3)
RAD_process(AIL,sample_buffer_ready,"d",1)
RAD_process(AIL,load_sample_buffer,"ddtd",4)
RAD_process(AIL,sample_buffer_info,"dpppp",5)
RAD_process(AIL,set_sample_position,"dd",2)
RAD_process(AIL,sample_position,"d",1)
RAD_process(AIL,set_sample_user_data,"ddd",3)
RAD_process(AIL,sample_user_data,"dd",2)
RAD_process(AIL,active_sample_count,"d",1)
RAD_process(AIL,digital_configuration,"dppp",4)

RAD_process(AIL,midiOutOpen,"ppd",3)
RAD_process(AIL,midiOutClose,"d",1)
RAD_process(AIL,MIDI_handle_release,"d",1)
RAD_process(AIL,MIDI_handle_reacquire,"d",1)
RAD_process(AIL,allocate_sequence_handle,"d",1)
RAD_process(AIL,release_sequence_handle,"d",1)
RAD_process(AIL,init_sequence,"dtd",3)
RAD_process(AIL,start_sequence,"d",1)
RAD_process(AIL,stop_sequence,"d",1)
RAD_process(AIL,resume_sequence,"d",1)
RAD_process(AIL,end_sequence,"d",1)
RAD_process(AIL,set_sequence_tempo,"ddd",3)
RAD_process(AIL,set_sequence_volume,"ddd",3)
RAD_process(AIL,set_sequence_loop_count,"dd",2)
RAD_process(AIL,sequence_status,"d",1)
RAD_process(AIL,sequence_tempo,"d",1)
RAD_process(AIL,sequence_volume,"d",1)
RAD_process(AIL,sequence_loop_count,"d",1)
RAD_process(AIL,set_XMIDI_master_volume,"dd",2)
RAD_process(AIL,XMIDI_master_volume,"d",1)
RAD_process(AIL,active_sequence_count,"d",1)
RAD_process(AIL,controller_value,"ddd",3)
RAD_process(AIL,channel_notes,"dd",2)
RAD_process(AIL,sequence_position,"dpp",3)
RAD_process(AIL,branch_index,"dd",2)
RAD_process(AIL,set_sequence_user_data,"ddd",3)
RAD_process(AIL,sequence_user_data,"dd",2)
RAD_process(AIL,lock_channel,"d",1)
RAD_process(AIL,release_channel,"dd",2)
RAD_process(AIL,map_sequence_channel,"ddd",3)
RAD_process(AIL,true_sequence_channel,"dd",2)
RAD_process(AIL,send_channel_voice_message,"ddddd",5)
RAD_process(AIL,send_sysex_message,"dp",2)
RAD_process(AIL,create_wave_synthesizer,"ddtd",4)
RAD_process(AIL,destroy_wave_synthesizer,"d",1)
                
RAD_process(AIL,register_SOB_callback,"dd",0x42)
RAD_process(AIL,register_EOB_callback,"dd",0x42)
RAD_process(AIL,register_EOS_callback,"dd",0x42)
RAD_process(AIL,register_EOF_callback,"dd",0x42)

RAD_process(AIL,set_timer_period,"dd",2)
RAD_process(AIL,set_timer_frequency,"dd",2)
RAD_process(AIL,set_timer_divisor,"dd",2)
RAD_process(AIL,register_timer,"d",0x41)
RAD_process(AIL,start_timer,"d",1)
RAD_process(AIL,start_all_timers,"",0)
RAD_process(AIL,set_timer_user,"dd",2)
RAD_process(AIL,stop_timer,"d",1)
RAD_process(AIL,stop_all_timers,"",0)
RAD_process(AIL,release_timer_handle,"d",1)
RAD_process(AIL,release_all_timers,"",0)

RAD_process(AIL,register_ICA_array,"dt",2)

RAD_process(AIL,register_prefix_callback,"dd",0x42)
RAD_process(AIL,register_trigger_callback,"dd",0x42)
RAD_process(AIL,register_sequence_callback,"dd",0x42)
RAD_process(AIL,register_beat_callback,"dd",0x42)
RAD_process(AIL,register_event_callback,"dd",0x42)
RAD_process(AIL,register_timbre_callback,"dd",0x42)

RAD_process(AIL,redbook_open,"d",1)
RAD_process(AIL,redbook_close,"d",1)
RAD_process(AIL,redbook_eject,"d",1)
RAD_process(AIL,redbook_retract,"d",1)
RAD_process(AIL,redbook_status,"d",1)
RAD_process(AIL,redbook_tracks,"d",1)
RAD_process(AIL,redbook_track,"d",1)
RAD_process(AIL,redbook_track_info,"ddpp",4)
RAD_process(AIL,redbook_id,"d",1)
RAD_process(AIL,redbook_position,"d",1)
RAD_process(AIL,redbook_play,"ddd",3)
RAD_process(AIL,redbook_stop,"d",1)
RAD_process(AIL,redbook_pause,"d",1)
RAD_process(AIL,redbook_resume,"d",1)
RAD_process(AIL,redbook_volume,"d",1)
RAD_process(AIL,redbook_set_volume,"dd",2)

RAD_process(AIL,quick_startup,"ddddd",5)
RAD_process(AIL,quick_shutdown,"",0)
RAD_process(AIL,quick_handles,"ppp",3)
RAD_process(AIL,quick_load,"p",1)
RAD_process(AIL,quick_load_mem,"td",2)
RAD_process(AIL,quick_copy,"d",1)
RAD_process(AIL,quick_unload,"d",1)
RAD_process(AIL,quick_play,"dd",2)
RAD_process(AIL,quick_halt,"d",1)
RAD_process(AIL,quick_load_and_play,"pdd",3)
RAD_process(AIL,quick_status,"d",1)
RAD_process(AIL,quick_set_speed,"dd",2)
RAD_process(AIL,quick_set_volume,"ddd",3)
RAD_process(AIL,quick_set_reverb,"dddd",4)
RAD_process(AIL,quick_set_ms_position,"dd",2)
RAD_process(AIL,quick_ms_position,"d",1)
RAD_process(AIL,quick_ms_length,"d",1)
RAD_process(AIL,quick_type,"d",1)

RAD_process(AIL,open_stream,"dpd",3)
RAD_process(AIL,close_stream,"d",1)
RAD_process(AIL,service_stream,"dd",2)
RAD_process(AIL,start_stream,"d",1)
RAD_process(AIL,pause_stream,"dd",2)
RAD_process(AIL,set_stream_volume,"dd",2)
RAD_process(AIL,set_stream_pan,"dd",2)
RAD_process(AIL,stream_volume,"d",1)
RAD_process(AIL,stream_pan,"d",1)
RAD_process(AIL,set_stream_playback_rate,"dd",2)
RAD_process(AIL,stream_playback_rate,"d",1)
RAD_process(AIL,set_stream_loop_count,"dd",2)
RAD_process(AIL,stream_loop_count,"d",1)
RAD_process(AIL,set_stream_loop_block,"ddd",3)
RAD_process(AIL,stream_status,"d",1)
RAD_process(AIL,set_stream_position,"dd",2)
RAD_process(AIL,stream_position,"d",1)
RAD_process(AIL,stream_info,"dpppp",5)
RAD_process(AIL,register_stream_callback,"dd",0x42)
RAD_process(AIL,auto_service_stream,"dd",2)
RAD_process(AIL,set_stream_user_data,"ddd",3)
RAD_process(AIL,stream_user_data,"dd",2)

RAD_process(AIL,set_sample_adpcm_block_size,"dd",2)
RAD_process(AIL,redbook_open_drive,"d",1)
RAD_process(AIL,sample_granularity,"d",1)

RAD_process(AIL,DLS_open,"ddpdddd",7)
RAD_process(AIL,DLS_close,"dd",2)
RAD_process(AIL,DLS_load_file,"dpd",3)
RAD_process(AIL,DLS_load_memory,"dtd",3)
RAD_process(AIL,DLS_unload,"dd",2)
RAD_process(AIL,DLS_compact,"d",1)
RAD_process(AIL,DLS_set_reverb,"dddd",4)
RAD_process(AIL,DLS_get_reverb,"dppp",4)
RAD_process(AIL,DLS_get_info,"dpp",3)

RAD_process(AIL,set_sample_ms_position,"dd",2)
RAD_process(AIL,sample_ms_position,"dpp",3)

RAD_process(AIL,set_sequence_ms_position,"dd",2)
RAD_process(AIL,sequence_ms_position,"dpp",3)

RAD_process(AIL,set_stream_ms_position,"dd",2)
RAD_process(AIL,stream_ms_position,"dpp",3)

RAD_process(AIL,set_stream_reverb,"dddd",4)
RAD_process(AIL,stream_reverb,"dppp",4)

RAD_process(AIL,open_input,"p",1)
RAD_process(AIL,close_input,"d",1)
RAD_process(AIL,get_input_info,"d",0x81)
RAD_process(AIL,set_input_state,"dd",2)
