//============================================================================
// PMF Player
//
// Copyright (c) 2019, Profoundic Technologies, Inc.
// All rights reserved.
//----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Profoundic Technologies nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL PROFOUNDIC TECHNOLOGIES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#include "pmf_player.h"
#if defined(ARDUINO_ARCH_AVR)
#include "pmf_data.h"
#include "TimerOne.h"

//---------------------------------------------------------------------------


//===========================================================================
// audio buffer
//===========================================================================
static pmf_audio_buffer<int16_t, 400> s_audio_buffer;
//---------------------------------------------------------------------------


//===========================================================================
// pmf_player
//===========================================================================
#ifndef PWM_PLAY
ISR(TIMER1_COMPA_vect) {
    PORTD = (uint8_t)s_audio_buffer.read_sample<uint16_t, 8>();
}
//----
#else
ISR(TIMER2_COMPA_vect) {
    //interrupt commands for TIMER 2 here
    uint8_t value8bit = s_audio_buffer.read_sample<uint16_t, 8>();
    //value8bit = s_audio_buffer.read_sample<uint16_t, 8>();
    //const uint8_t timer1PWMpin = 9;
    //Timer1.setPwmDuty(9, value8bit * 4);
    OCR1A=(uint16_t)value8bit;
}

#endif

uint32_t pmf_player::get_sampling_freq(uint32_t sampling_freq_) const
{
  return sampling_freq_;
}
//----

void pmf_player::start_playback(uint32_t sampling_freq_)
{

  // enable playback interrupt at given playback frequency
  //DDRD=0xff;
  s_audio_buffer.reset();
#ifndef PWM_PLAY
  TCCR1A=0;
  TCCR1B=_BV(CS10)|_BV(WGM12); // CTC mode 4 (OCR1A)
  TCCR1C=0;
  TIMSK1=_BV(OCIE1A);          // enable timer 1 counter A
  OCR1A=(16000000+sampling_freq_/2)/sampling_freq_;
#else
    //// TIMER 2 for interrupt frequency 22222.222222222223 Hz:
    //cli(); // stop interrupts
    //TCCR2A = 0; // set entire TCCR2A register to 0
    ////TCCR2B = 0; // same for TCCR2B
    //TCNT2 = 0; // initialize counter value to 0
    //// set compare match register for 22222.222222222223 Hz increments
    //OCR2A = 255; // = 16000000 / (8 * 22222.222222222223) - 1 (must be <256)
    //// turn on CTC mode
    //TCCR2A = (1 << WGM21);
    //// Set CS22, CS21 and CS20 bits for 8 prescaler
    //TCCR2B |= (0 << CS22) | (1 << CS21) | (0 << CS20);
    ////TCCR2B |= (0 << CS22) | (0 << CS21) | (1 << CS20);
    //// enable timer compare interrupt
    //TIMSK2 |= (1 << OCIE2A);
    //sei(); // allow interrupts
    cli();
  TCCR2B = 0;// same for TCCR2B
    TCCR2A = (1 << WGM21);// same for TCCR2B
  TCNT2 = 0;//initialize counter value to 0
  // set compare match register for 1khz increments
  //OCR2A = 128;// = (16*10^6) / (1000*64) - 1 (must be <256) <- bug
  // turn on CTC mode
  //TCCR2A |= (1 << WGM21);
  // Set CS21and and CS20 bit for 64 prescaler
  TCCR2B &= ~(1 << CS20);
  TCCR2B |= (1 << CS21);
  TCCR2B &= ~(1 << CS22);
  // enable timer compare interrupt

  OCR2A = 128;// <- bug: OCR2A 는 TCCR 밑에서 세팅해야 된다..
  TIMSK2 |= (1 << OCIE2A);
  sei();
#endif
}
//----

void pmf_player::stop_playback()
{
#ifndef PWM_PLAY
  TIMSK1=0;
#else
    TIMSK2 = 0;
#endif
}
//----

void pmf_player::mix_buffer(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
  int16_t *buffer_begin=(int16_t*)buf_.begin, *buffer_end=buffer_begin+num_samples_;
  audio_channel *channel=m_channels, *channel_end=channel+m_num_playback_channels;
  do
  {
    // check for active channel
    if(!channel->sample_speed)
      continue;

    // get channel attributes
    size_t sample_addr=(size_t)(m_pmf_file+pgm_read_dword(channel->smp_metadata+pmfcfg_offset_smp_data));
    uint16_t sample_len=pgm_read_word(channel->smp_metadata+pmfcfg_offset_smp_length);/*todo: should be dword*/
    uint16_t loop_len=pgm_read_word(channel->smp_metadata+pmfcfg_offset_smp_loop_length_and_panning);/*todo: should be dword*/
    uint8_t volume=(uint16_t(channel->sample_volume)*(channel->vol_env.value>>9))>>8;
    register uint8_t sample_pos_frc=channel->sample_pos;
    register uint16_t sample_pos_int=sample_addr+(channel->sample_pos>>8);
    register uint16_t sample_speed=channel->sample_speed;
    register uint16_t sample_end=sample_addr+sample_len;
    register uint16_t sample_loop_len=loop_len;
    register uint8_t sample_volume=volume;
    register uint8_t zero=0, upper_tmp;
#ifndef _MSC_VER
    asm volatile
    (
      "push %A[buffer_pos] \n\t"
      "push %B[buffer_pos] \n\t"

      "mix_samples_%=: \n\t"
      "lpm %[upper_tmp], %a[sample_pos_int] \n\t"
      "mulsu %[upper_tmp], %[sample_volume] \n\t"
      "mov %[upper_tmp], r1 \n\t"
      "lsl %[upper_tmp] \n\t"
      "sbc %[upper_tmp], %[upper_tmp] \n\t"
      "ld __tmp_reg__, %a[buffer_pos] \n\t"
      "add __tmp_reg__, r1 \n\t"
      "st %a[buffer_pos]+, __tmp_reg__ \n\t"
      "ld __tmp_reg__, %a[buffer_pos] \n\t"
      "adc __tmp_reg__, %[upper_tmp] \n\t"
      "st %a[buffer_pos]+, __tmp_reg__ \n\t"
      "add %[sample_pos_frc], %A[sample_speed] \n\t"
      "adc %A[sample_pos_int], %B[sample_speed] \n\t"
      "adc %B[sample_pos_int], %[zero] \n\t"
      "cp %A[sample_pos_int], %A[sample_end] \n\t"
      "cpc %B[sample_pos_int], %B[sample_end] \n\t"
      "brcc sample_end_%= \n\t"
      "next_sample_%=: \n\t"
      "cp %A[buffer_pos], %A[buffer_end] \n\t"
      "cpc %B[buffer_pos], %B[buffer_end] \n\t"
      "brne mix_samples_%= \n\t"
      "rjmp done_%= \n\t"

      "sample_end_%=: \n\t"
      /*todo: implement bidi loop support*/
      "sub %A[sample_pos_int], %A[sample_loop_len] \n\t"
      "sbc %B[sample_pos_int], %B[sample_loop_len] \n\t"
      "mov __tmp_reg__, %A[sample_loop_len] \n\t"
      "or __tmp_reg__, %B[sample_loop_len] \n\t"
      "brne next_sample_%= \n\t"
      "clr %A[sample_speed] \n\t"
      "clr %B[sample_speed] \n\t"

      "done_%=: \n\t"
      "clr r1 \n\t"
      "pop %B[buffer_pos] \n\t"
      "pop %A[buffer_pos] \n\t"

      :[sample_speed] "+l" (sample_speed)
      ,[sample_pos_frc] "+l" (sample_pos_frc)
      ,[sample_pos_int] "+z" (sample_pos_int)

      :[sample_end] "r" (sample_end)
      ,[sample_volume] "a" (sample_volume)
      ,[upper_tmp] "a" (upper_tmp)
      ,[zero] "r" (zero)
      ,[sample_loop_len] "l" (sample_loop_len)
      ,[buffer_pos] "e" (buffer_begin)
      ,[buffer_end] "l" (buffer_end)
    );
#endif
    // store values back to the channel data
    channel->sample_pos=(long(sample_pos_int-sample_addr)<<8)+sample_pos_frc;
    channel->sample_speed=sample_speed;
  } while(++channel!=channel_end);

  // advance buffer
  ((int16_t*&)buf_.begin)+=num_samples_;
  buf_.num_samples-=num_samples_;
}
//----

pmf_mixer_buffer pmf_player::get_mixer_buffer()
{
  return s_audio_buffer.get_mixer_buffer();
}
//---------------------------------------------------------------------------

//===========================================================================
#endif // ARDUINO_ARCH_AVR
