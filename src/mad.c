#ifndef __MAD_C
#define __MAD_C

#include "libmad/mad.h"
#include "stdio.h"
#include <stdint.h>

#include "radio.h"


#define     __IO    volatile

#define dbgprintf(a,...) chprintf(NULL,a,__VA_ARGS__)
//#define dbgprintf(a,...)

unsigned char alloc_decoder[23000];
unsigned char decoder_used=0;

#ifndef ALLOC_SIZE
#define ALLOC_SIZE 10000
#endif

unsigned char alloc_buffer[ALLOC_SIZE];
int alloc_free;
unsigned char *alloc_ptr;

void *mp3_malloc(int size)
{
 void *ptr;

 if ((decoder_used==0)&&(size>22000)&&(size<23000)) {decoder_used=1; return (void*)alloc_decoder;}

 if (alloc_free<size) return NULL;

 //dbgprintf("Malloc %d (%x,%d)\r\n",size,alloc_ptr,alloc_free);
 size=(size+15)&0xFFF0;

 alloc_free-=size;
 ptr=alloc_ptr;
 alloc_ptr+=size;
 return ptr;
}
 
void mp3_free(void *ptr)
{
 //dbgprintf("Free %d\r\n",alloc_free);
 if (ptr==(void*)alloc_decoder) decoder_used=0;
}

void *mp3_calloc(int num,int size)
{
 unsigned char *ptr;
 int i;
 //dbgprintf("Calloc %d %d -> ",num,size);
 ptr=mp3_malloc(size*num);
 if (!ptr) return NULL;
 for(i=0;i<num*size;i++) ptr[i]=0;
 return ptr;
}


struct buffer {
  unsigned char const *start;
  unsigned long length;
};

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

int frame=0;


//mad_flow input перенесен в radio.c для удобства

/*
static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct buffer *buffer = data;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  frame=0;

  return MAD_FLOW_CONTINUE;
}


*/

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

extern uint16_t output_buff1[2*1152];
extern uint16_t output_buff2[2*1152];

extern __IO int output_buffnb;

extern __IO uint32_t XferCplt;
extern __IO uint32_t AudioTotalSize,AudioRemSize;
extern __IO uint16_t *CurrentPos;

#define OFFSET 0

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
  uint16_t *output;

  signed sample;
  int currentbuff;

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  currentbuff=output_buffnb;
  while (output_buffnb==currentbuff) ;

#if 0
  frame++;
  dbgprintf("%d",currentbuff);
  if ((frame&31)==0) dbgprintf("\r%c",'\n');
#endif
 
  if (currentbuff==1) { output=output_buff1; }
                else  { output=output_buff2; }

  while (nsamples--) {

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    *output++=(sample+OFFSET);

    if (nchannels == 2) sample = scale(*right_ch++);
    *output++=(sample+OFFSET);

    //if (nsamples==100) dbgprintf("MP3 out sample[100] %d\r\n",sample);
  }

#if 0
   dbgprintf("AUDIO PLAY %x %d - %d %d %x\r\n", 
                   output_buff, (char *)output-(char *)output_buff,AudioTotalSize,AudioRemSize,CurrentPos);
#endif

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
/*
  struct buffer *buffer = data;

  dbgprintf("decoding error 0x%04x (%s) at byte offset %u\r\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);
*/
  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

int decode(unsigned char const *start, unsigned long length)
{
  struct buffer buffer;
  struct mad_decoder decoder;
  int result;

  /* initialize our private message structure */

  buffer.start  = start;
  buffer.length = length;

  /* configure input, output, and error functions */

  mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);

  /* start decoding */

  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

  /* release the decoder */

  mad_decoder_finish(&decoder);

  //dbgprintf("Decoding return %d\r\n",result);

  return result;
}

#endif
