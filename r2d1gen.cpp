/*

Copyright (C) 2003-2005 Franz Kruhm.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*      LAST MODIFIED BY: FRANZ KRUHM - 23 July 2004    */
/*      LAST MODIFICATIONS: added r2_irand()            */
/*                          midf between minf and maxf  */
/*                          audio = new short[...]      */
/*                          fixed fshift (may need adj) */
/*                          added var check before print*/
/*                          tone angles in array        */
/*                          can specify max poly in rand*/
/*                          can turn off fuzz in rand   */
/*                          curses support              */
/*                          version number 0.95         */

#define R2D1_VERSION_NUM    0.95

#include <stdio.h>
#include <math.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#define USE_CURSES 

#ifdef USE_CURSES 
#include <curses.h>
#endif


//      --- Definitions ---
#define MAX_SAMPLE_RATE     96000
#define MAX_POLY            255
#define DEFAULT_VGPS        77
#define DEFAULT_VPOLY       6
#define DEFAULT_VMAXF       22050
#define DEFAULT_VMIDF       800
#define DEFAULT_VMINF       20
#define DEFAULT_VTSEC       30
#define DEFAULT_VRVAR       40
#define DEFAULT_VFUZZ       1
#define DEFAULT_VFADE       0
#define DEFAULT_VFSH        0
#define DEFAULT_VRATE       MAX_SAMPLE_RATE / 2
#define DEFAULT_VCLR        1
#define DEFAULT_VRANDOM     0
#define DEFAULT_VFN         "r2d1_out.wav"


//      --- Global variables ---
short *audio;
double *angles;
FILE *output;
unsigned int v_gps;
unsigned int v_poly;
unsigned int v_maxf;
unsigned int v_minf;
unsigned int v_midf;
unsigned int v_tsec;
unsigned int v_rvar;
unsigned int v_fuzz;
unsigned int v_fsh;
unsigned int v_fade;
unsigned int v_rate;
unsigned int v_random;
unsigned int v_clr;
char v_fn[255];
unsigned int iloops;
#ifdef USE_CURSES
WINDOW *cw;
#endif


//      --- Backup variables ---
unsigned int __v_minf;
unsigned int __v_midf;
unsigned int __v_maxf;
unsigned int __v_poly;
unsigned int __v_fuzz;
unsigned int __v_gps;


/*
    ========================
    r2_rand(): returns a
        random int between
        imin and imax 
    ========================
*/
int r2_irand(int imin, int imax)
{
        return imin + rand() / ((RAND_MAX+1) / (imax+1-imin));
}


/*
    ========================
    r2_printf(): printf 
    ========================
*/
void r2_printf(char *fmt, ...)
{
        va_list argptr;
        char msg[255];
        va_start(argptr, fmt);
        vsprintf(msg, fmt, argptr);
        va_end(argptr);
        #ifndef USE_CURSES
        printf("%s", msg);
        #else
        printw("%s", msg);
        refresh();
        #endif
}


/*
    ========================
    r2_reprintf(): printf on
        the same line.
    ========================
*/
void r2_reprintf(char *fmt, ...)
{
        va_list argptr;
        unsigned int x;
        unsigned int y;
        char msg[255];
        va_start(argptr, fmt);
        vsprintf(msg, fmt, argptr);
        va_end(argptr);
        #ifndef USE_CURSES
        printf("%s", msg);
        #else
        getyx(cw, x, y);
        mvprintw(x-1, 0, "%s", msg);
        refresh();
        #endif
}


/*
    ========================
    CreateWAV(): Create WAV
        file
    ========================
*/
void CreateWAV(char *filename, unsigned int rate, unsigned int bits, unsigned int channels, unsigned int seconds)
{
        unsigned long totalsize, bytespersec;
        output = fopen(filename, "wb");
        if (output == NULL)
        {
                r2_printf("\n\nCANNOT CREATE WAV FILE! \n\n");
                exit(2);
        }    
        fprintf(output, "RIFF");
        totalsize = 2 * (rate * seconds) + 36;
        fputc((totalsize & 0x000000FF), output);
        fputc((totalsize & 0x0000FF00) >> 8, output);
        fputc((totalsize & 0x00FF0000) >> 16, output);
        fputc((totalsize & 0xFF000000) >> 24, output);
        fprintf(output, "WAVE");
        fprintf(output, "fmt ");
        fputc(16, output);
        fputc(0, output);
        fputc(0, output);
        fputc(0, output);
        fputc(1, output);
        fputc(0, output);
        fputc(1, output);
        fputc(0, output);
        fputc((rate & 0x000000FF), output);
        fputc((rate & 0x0000FF00) >> 8, output);
        fputc((rate & 0x00FF0000) >> 16, output);
        fputc((rate & 0xFF000000) >> 24, output);
        bytespersec = 2 * rate;
        fputc((bytespersec & 0x000000FF), output);
        fputc((bytespersec & 0x0000FF00) >> 8, output);
        fputc((bytespersec & 0x00FF0000) >> 16, output);
        fputc((bytespersec & 0xFF000000) >> 24, output);
        fputc(2, output);
        fputc(0, output);
        fputc(16, output);
        fputc(0, output);
        fprintf(output, "data");
        totalsize = 2 * (rate * seconds);
        fputc((totalsize & 0x000000FF), output);
        fputc((totalsize & 0x0000FF00) >> 8, output);
        fputc((totalsize & 0x00FF0000) >> 16, output);
        fputc((totalsize & 0xFF000000) >> 24, output);
}


/*
    ========================
    WriteWAV(): Write buffer
        to WAV file
    ========================
*/
inline void WriteWAV(void)
{
        unsigned int sz = v_rate/v_gps;
        if (sz > 0 && output != NULL)
        {
                fwrite(audio, sizeof(short), sz, output);
        }
}


/*
    ========================
    CloseWAV(): Close the
        WAV file
    ========================
*/
void CloseWAV(void)
{
        if (output != NULL) fclose(output);
}


/*
    ========================
    CheckVars(): Check vars
        for bad values
    ========================
*/
inline void CheckVars(void)
{
        if (v_minf == 0) v_minf = 1;
        if (v_gps == 0) v_gps = 1;
        if (v_poly > MAX_POLY) v_poly = MAX_POLY;
        if (v_rate > MAX_SAMPLE_RATE) v_rate = MAX_SAMPLE_RATE;
        if (v_rate < 11025) v_rate = 11025;
        if (v_maxf < v_minf) v_maxf = v_minf;
        if (v_maxf > (v_rate / 2)) v_maxf = v_rate / 2;
        if (v_minf > v_maxf) v_minf = (v_maxf / 2);
        if (v_midf < v_minf) v_midf = v_minf;
        if (v_midf > v_maxf) v_midf = (v_maxf / 2);
        if (v_tsec == 0) 
        {
                r2_printf("TSec set to 0 !!!\n");
                exit(1);
        }
        if (v_rvar == 0) v_rvar = 1;
}


/*
    ========================
    RandomizeVars(): vars 
        are randomized
    ========================
*/
inline void RandomizeVars(void)
{
        if (r2_irand(0, 100) <= 50)
        {
                v_fsh = 1;
        }
        else v_fsh = 0;
        if (r2_irand(0, 100) <= 20)
        {
                v_fade = 1;
        }
        else v_fade = 0;
        if (r2_irand(0, 100) <= 30 && __v_fuzz == 1)
        {
                v_fuzz = 1;
        }
        else v_fuzz = 0;
        v_rvar = r2_irand(0, 255);
        v_gps = r2_irand(1, __v_gps);
        v_poly = r2_irand(0, __v_poly);
        v_minf = r2_irand(0, __v_minf);
        v_midf = r2_irand(__v_minf, __v_midf);
        v_maxf = r2_irand(__v_midf, __v_maxf);
        CheckVars();
}


/*
    ========================
    SignalClear(): Clear the
        audio buffer
    ========================
*/
inline void SignalClear(void)
{
        unsigned int indx;
        indx = 0;
        while (indx < v_rate/v_gps)
        {
                audio[indx] = 0;
                indx++;
        }
}


/*
    ========================
    SignalTone(): Generate
        tone and mix in the
        audio buffer.
    ========================
*/
inline void SignalTone(unsigned int freq, int amp)
{
        int i;
        double d;
        unsigned int indx;
        double dangle;
        double fshift = 0.0;
        if (v_clr == 1) angles[iloops] = 0.0f;
        indx = 0;
        if (amp == 0) amp = 0;
        if (freq < v_maxf && freq > v_minf)
        {
                if (v_fsh == 0) dangle = 3.14159*2.0*(double)freq/(double)v_rate;
                while (indx < (v_rate/v_gps)+1)
                {
                        if (v_fsh == 1) 
                        {
                                if (v_gps > 10) 
                                {
                                        fshift += ((0.000066 * v_maxf) * (r2_irand(0, 10)) * (v_gps/10));
                                }
                                else fshift += ((0.000066 * v_maxf) * r2_irand(0, 10));
                                dangle = 3.14159*2.0*(double)(freq-fshift)/(double)v_rate;
                        }
                        d = sinf(angles[iloops])*32767.5;
                        i = d/amp;
                        audio[indx] = (audio[indx] / 2) + (i / 2);
                        angles[iloops] += dangle;
                        indx++;
                }
        }
} 


/*
    ========================
    SignalFuzz(): Fuzz the 
        audio buffer
    ========================
*/
inline void SignalFuzz(short *data, unsigned long len, int amp)
{
        unsigned long l;
        int x;
        l = 0;
        if (amp == 0) amp = 1.0f;
        while (l < len)
        {
                x=((r2_irand(0, 5))+1)%((r2_irand(0, 5))+1)+((r2_irand(0, 15))+1)%((r2_irand(0, 15))+1);
                data[l]=(data[l]^2)+(data[l+r2_irand(0, 10)]/2)*(float)(x/amp);
                l++;
        }
}


/*
    ========================
    SignalAmp(): Amplify
        the audio buffer
    ========================
*/
inline void SignalAmp(short *data, unsigned long len, int amp)
{
        unsigned long l;
        double ampor;
        double rampin = 0.0;
        double rampfact;
        int done = 0;
        l = 0;
        ampor = amp;
        if (amp == 0) ampor = 1.0;
        rampfact = ampor/1000;
        while (l < len)
        {
                done = 0;
                if (v_fade == 1)
                {
                        if (l < 1000)
                        {
                                data[l] *= rampin;
                                rampin += rampfact;
                                done = 1;
                        }
                        if (l > len - 1000)
                        {
                                data[l] *= rampin;
                                rampin -= rampfact;
                                done = 1;
                        }
                }
                if (done == 0) data[l] *= ampor;
                l++;
        }
}


/*
    ========================
    SignalSynth(): Main 
        synth loop
    ========================
*/
void SignalSynth(void)
{
        unsigned int sec = 0;
        unsigned int loops = 0;
        iloops = 0;
        unsigned int freq;
        r2_printf("\nExecing...\n");
        while (sec < v_tsec)
        {
                while (loops < v_gps)
                {
                        while (iloops < v_poly)
                        {
                                freq = (r2_irand(v_minf, v_midf) + r2_irand(v_midf, v_maxf)) / 2;
                                SignalTone(freq, r2_irand(0, v_rvar));
                                freq /= 5;
                                if (freq < v_minf) freq = v_minf;
                                SignalTone(freq, r2_irand(0, v_rvar));
                                freq *= 10;
                                if (freq > v_maxf) freq = v_maxf;
                                SignalTone(freq, r2_irand(0, v_rvar));
                                if (v_fuzz == 1) SignalFuzz(audio, v_rate/v_gps, r2_irand(0, v_rvar));
                                iloops++;
                        }
                        SignalAmp(audio, v_rate/v_gps, 1.8);
                        #ifdef WIN32
                        srand(timeGetTime());
                        #else
                        srand(time(0));
                        #endif
                        WriteWAV();
                        if (v_random == 1) RandomizeVars();
                        if (v_clr == 1) SignalClear();
                        iloops = 0;
                        loops++;
                }
                loops = iloops = 0;
                sec++;
                r2_reprintf("Synth'd %i out of %i\n", sec, v_tsec);
        }
}


/*
    ========================
    main(): program start
    ========================
*/
int main(int argc, char *argv[])
{
        #ifdef USE_CURSES
        cw = initscr();
        #endif
        v_gps = DEFAULT_VGPS;
        v_poly = DEFAULT_VPOLY;
        v_maxf = DEFAULT_VMAXF;
        v_minf = DEFAULT_VMINF;
        v_midf = DEFAULT_VMIDF;
        v_tsec = DEFAULT_VTSEC;
        v_rvar = DEFAULT_VRVAR;
        v_fuzz = DEFAULT_VFUZZ;
        v_fade = DEFAULT_VFADE;
        v_fsh = DEFAULT_VFSH;
        v_rate = DEFAULT_VRATE;
        v_clr = DEFAULT_VCLR;
        v_random = DEFAULT_VRANDOM;
        strcpy(v_fn, DEFAULT_VFN);
        r2_printf("\nR2D1 Command-line Grain Experimental Synth v%.2f\n", R2D1_VERSION_NUM);
        r2_printf("(C)copyright 2003-2005 Franz Kruhm\nzanfr@photoreal.com\nuse -h for help\n");
        if (argc > 1)
        {
                int dummy = 1;
                while (dummy < argc)
                {
                        if (strcmp("-vgps", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_gps = atoi(argv[dummy]);
                        }
                        if (strcmp("-vpoly", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_poly = atoi(argv[dummy]);
                        }
                        if (strcmp("-vmaxf", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_maxf = atoi(argv[dummy]);
                        }
                        if (strcmp("-vminf", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_minf = atoi(argv[dummy]);
                        }
                        if (strcmp("-vmidf", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_midf = atoi(argv[dummy]);
                        }
                        if (strcmp("-vtsec", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_tsec = atoi(argv[dummy]);
                        }
                        if (strcmp("-vrvar", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_rvar = atoi(argv[dummy]);
                        }
                        if (strcmp("-vfuzz", argv[dummy]) == 0)
                        {
                                v_fuzz = 0;
                        }
                        if (strcmp("-vfn", argv[dummy]) == 0)
                        {
                                dummy++;
                                strcpy(v_fn, argv[dummy]);
                        }
                        if (strcmp("-vfade", argv[dummy]) == 0)
                        {
                                v_fade = 1;
                        }
                        if (strcmp("-vfsh", argv[dummy]) == 0)
                        {
                                v_fsh = 1;
                        }
                        if (strcmp("-vrate", argv[dummy]) == 0)
                        {
                                dummy++;
                                v_rate = atoi(argv[dummy]);
                        }
                        if (strcmp("-vrandom", argv[dummy]) == 0)
                        {
                                v_random = 1;
                        }
                        if (strcmp("-vclr", argv[dummy]) == 0)
                        {
                                v_clr = 0;
                        }
                        if (strcmp("-h", argv[dummy]) == 0)
                        {
                                r2_printf("\n-h displays help\n");
                                r2_printf("-vgps [grains] sets the number of grains per sec\n");
                                r2_printf("-vpoly [poly] sets the poly\n");
                                r2_printf("-vmaxf [hz] max frequency\n");
                                r2_printf("-vminf [hz] min frequency\n");
                                r2_printf("-vmidf [hz] sets the middle frequency\n");
                                r2_printf("-vtsec [seconds] sets the duration\n");
                                r2_printf("-vrvar [number] sets the variation for random\n");
                                r2_printf("-vfuzz disables the fuzz\n");
                                r2_printf("-vfn sets the output filename\n");
                                r2_printf("-vfsh hard pitch shift in grains\n");
                                r2_printf("-vfade fade in/out grains\n");
                                r2_printf("-vrate [hertz] specify sampling rate\n");
                                r2_printf("-vclr disable audio buffer and angles clear\n");
                                r2_printf("-vrandom randomizes variables (overrides vfsh, vfade)\n");
                                #ifdef USE_CURSES
                                endwin();
                                #endif
                                exit(1);
                        }
                        dummy++;
                }
        }
        audio = new short[MAX_SAMPLE_RATE];
        angles = new double[MAX_POLY];
        CreateWAV(v_fn, v_rate, 16, 1, v_tsec);
        CheckVars();
        if (v_random == 0)
        {
                r2_printf("\nv_gps: %i\n", v_gps);
                r2_printf("v_poly: %i\n", v_poly);
                r2_printf("v_maxf: %i\n", v_maxf);
                r2_printf("v_minf: %i\n", v_minf);
                r2_printf("v_midf: %i\n", v_midf);
                r2_printf("v_rvar: %i\n", v_rvar);
                r2_printf("v_fuzz: %i\n", v_fuzz);
                r2_printf("v_fade: %i\n", v_fade);
                r2_printf("v_fsh:  %i\n", v_fsh);   
        }
        else
        {
                __v_minf = v_minf;
                __v_midf = v_midf;
                __v_maxf = v_maxf;
                __v_poly = v_poly;
                __v_fuzz = v_fuzz;
                __v_gps = v_gps;
                r2_printf("\n:RANDOM MODE:\n");
                r2_printf("v_gps:  %i\n", __v_gps);
                r2_printf("v_poly: %i\n", __v_poly);
                r2_printf("v_minf: %i\n", __v_minf);
                r2_printf("v_midf: %i\n", __v_midf);
                r2_printf("v_maxf: %i\n", __v_maxf);
                r2_printf("v_fuzz: %i\n", __v_fuzz);
        }
        r2_printf("v_tsec: %i\n", v_tsec);
        r2_printf("v_rate: %i\n", v_rate);
        r2_printf("v_clr:  %i\n", v_clr);
        SignalSynth();
        CloseWAV();
        delete audio;
        delete angles;
        #ifdef USE_CURSES
        endwin();
        #endif
        return 0;
}
/*                End of file : r2d1gen.cpp                */

