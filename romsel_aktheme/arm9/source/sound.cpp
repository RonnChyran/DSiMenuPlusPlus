#include "sound.h"

#include "systemfilenames.h"
#include "common/dsimenusettings.h"
// #include "soundbank_bin.h"
#include "streamingaudio.h"
#include "string.h"
#include <algorithm>

#define SFX_STARTUP		0
#define SFX_WRONG		1
#define SFX_LAUNCH		2
#define SFX_STOP		3
#define SFX_SWITCH		4
#define SFX_SELECT		5
#define SFX_BACK		6

#define MSL_NSONGS		0
#define MSL_NSAMPS		7
#define MSL_BANKSIZE	7


extern volatile s16 fade_counter;
extern volatile bool fade_out;

extern volatile s16* play_stream_buf;
extern volatile s16* fill_stream_buf;

// Number of samples filled into the fill buffer so far.
volatile u32 filled_samples = 0;

extern volatile bool fill_requested;
extern volatile u32 samples_left_until_next_fill;
extern volatile u32 streaming_buf_ptr;
// volatile u32 fill_count;
#define SAMPLES_USED (STREAMING_BUF_LENGTH - samples_left)
#define REFILL_THRESHOLD STREAMING_BUF_LENGTH >> 2

// extern char debug_buf[256];

extern volatile u32 sample_delay_count;

volatile char SFX_DATA[0x7D000] = {0};
mm_word SOUNDBANK[MSL_BANKSIZE] = {0};

SoundControl::SoundControl() 
	: stream_is_playing(false), use_soundbank(false), use_stream(false), stream_source(NULL), startup_sample_length(0)
 {

	sys.fifo_channel = FIFO_MAXMOD;
	
	FILE* soundbank_file;

	soundbank_file = fopen(std::string(SFN_SOUND_EFFECTBANK).c_str(), "rb");
	
	if (soundbank_file) {
		fread((void*)SFX_DATA, 1, sizeof(SFX_DATA), soundbank_file);
		fclose(soundbank_file);
		use_soundbank = true;
		
	}

	if (use_soundbank) {
		sys.mem_bank = SOUNDBANK;
		sys.mod_count = MSL_NSONGS;
		sys.samp_count = MSL_NSAMPS;
		// Since SFX_STARTUP is the first sample, it begins at 0x10 after the
		// *maxmod* header. Subtract the size of the sample header,
		// and divide by two to get length in samples.
		// https://github.com/devkitPro/mmutil/blob/master/source/msl.c#L80
		startup_sample_length = (((*(u32*)(SFX_DATA + 0x10)) - 20) >> 1);
	} else {
		sys.mem_bank = 0;
		sys.mod_count = 0;
		sys.samp_count = 0;
	}

	mmInit(&sys);
	// sprintf(debug_buf, "Read sample length %li for startup", startup_sample_length);
    // nocashMessage(debug_buf);

	if (use_soundbank) {
		mmSoundBankInMemory((mm_addr)SFX_DATA);
		mmLoadEffect(SFX_STARTUP);

		mus_startup = {
					{SFX_STARTUP},	   // id
					(int)(1.0f * (1 << 10)), // rate
					0,			     // handle
					255,		     // volume
					128,		     // panning
		};
	// mmLoadEffect(SFX_LAUNCH);
	// mmLoadEffect(SFX_SELECT);
	// mmLoadEffect(SFX_STOP);
	// mmLoadEffect(SFX_WRONG);
	// mmLoadEffect(SFX_BACK);
	// mmLoadEffect(SFX_SWITCH);
	// 	// mmLoadEffect(SFX_MENU);

	// 	snd_launch = {
	// 		{SFX_LAUNCH},	    // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};
	// 	snd_select = {
	// 		{SFX_SELECT},	    // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};
	// 	snd_stop = {
	// 		{SFX_STOP},		     // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};
	// 	snd_wrong = {
	// 		{SFX_WRONG},	     // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};
	// 	snd_back = {
	// 		{SFX_BACK},		     // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};
	// 	snd_switch = {
	// 		{SFX_SWITCH},	    // id
	// 		(int)(1.0f * (1 << 10)), // rate
	// 		0,			     // handle
	// 		255,		     // volume
	// 		128,		     // panning
	// 	};

	
	}

	stream_source = fopen(std::string(SFN_SOUND_BG).c_str(), "rb");
	if (stream_source) {
		fseek(stream_source, 0, SEEK_SET);
		// Prep the first section of the stream
		fread((void*)play_stream_buf, sizeof(s16), STREAMING_BUF_LENGTH, stream_source);

		// Fill the next section premptively
		fread((void*)fill_stream_buf, sizeof(s16), STREAMING_BUF_LENGTH, stream_source);
		use_stream = true;

	}

	stream.sampling_rate = 16000;	 // 16000HZ
	stream.buffer_length = 1600;	  // should be adequate
	stream.callback = on_stream_request;  
	stream.format = MM_STREAM_16BIT_MONO; // select format
	stream.timer = MM_TIMER1;	     // use timer0
	stream.manual = false;	      // manual filling
}

mm_sfxhand SoundControl::playStartup() { return use_soundbank ? mmEffectEx(&mus_startup) : 0; }

// mm_sfxhand SoundControl::playLaunch() { return mmEffectEx(&snd_launch); }
// mm_sfxhand SoundControl::playSelect() { return mmEffectEx(&snd_select); }
// mm_sfxhand SoundControl::playBack() { return mmEffectEx(&snd_back); }
// mm_sfxhand SoundControl::playSwitch() { return mmEffectEx(&snd_switch); }
// mm_sfxhand SoundControl::playStop() { return mmEffectEx(&snd_stop); }
// mm_sfxhand SoundControl::playWrong() { return mmEffectEx(&snd_wrong); }

void SoundControl::beginStream() {
	if (!use_stream) return;
	// open the stream
	stream_is_playing = true;
	mmStreamOpen(&stream);
	SetYtrigger(0);
}

void SoundControl::stopStream() {
	if (!use_stream) return;
	stream_is_playing = false;
	mmStreamClose();
}

void SoundControl::fadeOutStream() {
	if (!use_stream) return;
	fade_out = true;
}

void SoundControl::cancelFadeOutStream() {
	if (!use_stream) return;
	fade_out = false;
	fade_counter = FADE_STEPS;
}

void SoundControl::setStreamDelay(u32 delay) {
	if (!use_stream) return;
	sample_delay_count = delay;
}

// Samples remaining in the fill buffer.
#define SAMPLES_LEFT_TO_FILL ((STREAMING_BUF_LENGTH - filled_samples) % STREAMING_BUF_LENGTH + 1)

// Samples that were already streamed and need to be refilled into the buffer.
#define SAMPLES_TO_FILL ((streaming_buf_ptr - filled_samples) % STREAMING_BUF_LENGTH + 1)

// Updates the background music fill buffer
// Fill the amount of samples that were used up between the
// last fill request and this.
volatile void SoundControl::updateStream() {
	if (!use_stream) return;
	if (!stream_is_playing) return;
	if (fill_requested && filled_samples < STREAMING_BUF_LENGTH) {
		
		// Reset the fill request
		fill_requested = false;
		long unsigned int instance_filled = 0;

		// Either fill the max amount, or fill up the buffer as much as possible.
		long unsigned int instance_to_fill = std::min(SAMPLES_LEFT_TO_FILL, SAMPLES_TO_FILL);

		// If we don't read enough samples, loop from the beginning of the file.
		instance_filled = fread((s16*)fill_stream_buf + filled_samples, sizeof(s16), instance_to_fill, stream_source);		
		if (instance_filled <  instance_to_fill) {
			fseek(stream_source, 0, SEEK_SET);
			instance_filled += fread((s16*)fill_stream_buf + filled_samples + instance_filled,
				 sizeof(s16), (instance_to_fill - instance_filled), stream_source);
		}

		filled_samples += instance_filled;

		/* This part is a bit iffy. 
		 * SAMPLES_PER_FILL is a bit of a misnomer, as it doesn't actually fill this many samples. 
		 * Rather, this is how many samples elapse until the next fill request.
		 * 
		 * Setting this to SAMPLES_PER_FILL is a good approximation for when the next
		 * chunk is needed, so we just do that.
		 */ 
		samples_left_until_next_fill = SAMPLES_PER_FILL;
		
		// // Debug stuff.
		// fill_count++;
		// sprintf(debug_buf, "Loaded  %li samples, currently %li filled, fill number %i", instance_filled, filled_samples, fill_count);
    	// nocashMessage(debug_buf);
	
	} else if (filled_samples >= STREAMING_BUF_LENGTH) {
		filled_samples = 0;
		// fill_count = 0;
	}

}