// Adapted from: https://forum.pjrc.com/archive/index.php/t-27832.html

#include "arm_math.h"

#define FFTSIZE 256

int16_t s1[FFTSIZE], s2[FFTSIZE];
int16_t fft_buf[FFTSIZE * 2];
int32_t ifft_buf[FFTSIZE * 2];
arm_cfft_radix4_instance_q15 fft_inst;
arm_cfft_radix4_instance_q31 ifft_inst;

void setup() {
  Serial.begin(9600);
  arm_cfft_radix4_init_q15(&fft_inst, FFTSIZE, 0, 1);
  arm_cfft_radix4_init_q31(&ifft_inst, FFTSIZE, 1, 1);
}

void loop() {
  // generate two signals with random data, s2 is delayed s1
  unsigned offset = random(FFTSIZE / 4);
  for (unsigned i = 0; i < FFTSIZE + offset; i++) {
    int16_t val = ((int16_t) random(65536));
    if (i < FFTSIZE)
    s1[i] = val;
    if (i > offset)
    s2[i - offset] = val;
  }

  // Copy samples interleaved to FFT buffer s1 in real part, s2 in complex part
  // ie. fftbuf = s1 + j * s2
  for (int i = 0; i < FFTSIZE; i++)
    ((int32_t *)fft_buf)[i] = (s2[i] << 16) | s1[i];
    
  elapsedMicros us; // start timing
  
  // Perform FFT
  arm_cfft_radix4_q15(&fft_inst, fft_buf);

  // Untangle fft of both signals and calculate conj(fft(s1)) * fft(s2)
  // Treat DC point special
  int16_t fs1_re, fs1_im, fs2_re, fs2_im;
  fs1_re = fft_buf[0]; // fs1_im = 0
  fs2_re = fft_buf[0]; // fs2_im = 0
  ifft_buf[0] = fs1_re * fs2_re;
  ifft_buf[1] = 0;
  // Then handle most other points
  int16_t *fft_fwd = fft_buf + 2;
  int16_t *fft_rev = fft_buf + 2 * (FFTSIZE - 1);
  for (size_t i = 2; i < FFTSIZE; i += 2, fft_fwd += 2, fft_rev -= 2) {
    fs1_re = (fft_fwd[0] + fft_rev[0]) >> 1;
    fs1_im = (fft_fwd[1] - fft_rev[1]) >> 1;
    fs2_re = (fft_rev[1] + fft_fwd[1]) >> 1;
    fs2_im = (fft_rev[0] - fft_fwd[0]) >> 1;
    // calculate conj(fft(s1)) * fft(s2)
    int32_t cfs1_fs2_re = (fs1_re * fs2_re + fs1_im * fs2_im);
    int32_t cfs1_fs2_im = (fs1_re * fs2_im - fs1_im * fs2_re);
    ifft_buf[i] = ifft_buf[2 * FFTSIZE - i] = cfs1_fs2_re;
    ifft_buf[i + 1] = cfs1_fs2_im;
    ifft_buf[2 * FFTSIZE - i + 1] = -cfs1_fs2_im;
  }
  // Finally handle Nyquist point
  fs1_re = fft_fwd[0]; // fs1_im = 0
  fs2_re = fft_fwd[1]; // fs2_im = 0
  ifft_buf[FFTSIZE] = fs1_re * fs2_re;
  ifft_buf[FFTSIZE + 1] = 0;
  
  // Perform inverse FFT
  arm_cfft_radix4_q31(&ifft_inst, ifft_buf);
  
  uint32_t duration = us; // end of timing
  
  // The output data is in every second element of ifft_buf
  
  // As an example, find the cross-cor peak
  int peak_offset = 0;
  int32_t peak_value = ifft_buf[0];
  for (size_t i = 1, j = 2; i < FFTSIZE; i++, j += 2) {
    if (ifft_buf[j] > peak_value) {
      peak_value = ifft_buf[j];
      peak_offset = i;
    }
  }
  if (peak_offset > FFTSIZE / 2)
  peak_offset -= FFTSIZE;
  
  Serial.print("Generated offset: ");
  Serial.println(offset); 
  Serial.print("Calculated offset: ");
  Serial.println(-peak_offset); // delay implies negative
  Serial.print("It took ");
  Serial.print(duration);
  Serial.println(" us\n");
  
  delay(500);
}
