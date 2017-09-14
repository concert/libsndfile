/*
** Copyright (C) 2002-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	"sfconfig.h"

#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include    <mpg123.h>

#include	"sndfile.h"
#include	"sfendian.h"
#include	"common.h"

/*------------------------------------------------------------------------------
** Macros to handle big/little endian issues.
*/

/*------------------------------------------------------------------------------
** Typedefs.
*/

/*------------------------------------------------------------------------------
** Private static functions.
*/
static	int		mp3_close		(SF_PRIVATE *psf) ;
static  int     mp3_read_header (SF_PRIVATE * psf, mpg123_handle * decoder) ;
static  sf_count_t mp3_read_2s  (SF_PRIVATE * psf, short * ptr, sf_count_t len) ;
static  ssize_t mp3_read_sf_handle (void * handle, void * buffer, size_t bytes) ;
static off_t mp3_seek_sf_handle (void * handle, off_t offset, int whence) ;
static int mp3_format_to_encoding(int encoding) ;

// FIXME: This initialisation should have a better hook
static int mpg123_initialised = 0 ;

/*------------------------------------------------------------------------------
** Public function.
*/

// FIXME: use mpg123 error string reporting
int
mp3_open (SF_PRIVATE * psf)
{
    int error = 0;
    int decoder_err = MPG123_OK;
    mpg123_handle * decoder;
    printf("IN MP3 OPEN BITCHES\n");
    printf("MP3 Initial fileoffset: %li\n", psf->fileoffset);

    psf->codec_data = NULL;

    // TODO: writing!
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
		return SFE_UNIMPLEMENTED ;

    if (mpg123_initialised == 0) {
        int decoder_init_err = mpg123_init();
        // FIXME: find somewhere to put mpg123_exit() call
        if (decoder_init_err != MPG123_OK) {
            psf_log_printf(psf, "Failed to init mpg123.\n");
            return SFE_UNIMPLEMENTED ; // FIXME: semantically wrong return code
        }
    }
    mpg123_initialised++;

    printf("PANDASPANDASPANDAS\n");
    decoder = mpg123_new(NULL, &decoder_err);
    printf("ChairsWithFlares\n");

    // I think: mpg123_replace_reader_handle might be better than what I have now
    if (decoder_err == MPG123_OK) {
        decoder_err = mpg123_replace_reader_handle(
            decoder, mp3_read_sf_handle, mp3_seek_sf_handle, NULL);
    }
    // This has to be set before the handle open or mysterious things will
    // happen on seek. As for whether this is the right value :shrug:.
    //psf->fileoffset = 0;
    printf("Itchy nose\n");
    if (decoder_err == MPG123_OK) {
        decoder_err = mpg123_open_handle(decoder, psf);
    }

    printf("LemonTable\n");
    if (decoder_err == MPG123_OK) {
        decoder_err = mp3_read_header(psf, decoder);
    }

    // Something tells me this isn't quite right (namely that I haven't a clue
    // what these do) - the definitions are in src/common.h
    psf->dataoffset = 0;
    psf->datalength = psf->filelength ;
    psf->blockwidth = 0;

    printf("lost_cheese %i\n", decoder_err);
    if (decoder_err != MPG123_OK) {
        //mp3_close(psf);
        psf_log_printf(psf, "Failed to initialise mp3 decoder.\n");
        return SFE_UNIMPLEMENTED ; // FIXME: semantically wrong return code
    }
    psf->codec_data = decoder;
	psf->container_close = mp3_close ;
    // TODO: seeking and other reads
    psf->read_short = mp3_read_2s;

    printf("melon face %i\n", error);
    return error;
}

/*------------------------------------------------------------------------------
*/

static int
mp3_close (SF_PRIVATE * psf)
{
    mpg123_handle * decoder = psf->codec_data;
    if (decoder != NULL) {
        // mpg123_close(decoder); <- Not sure if we need this?
        mpg123_delete(decoder);
        psf->codec_data = NULL;  // The SF can be reused (apparently)
    }
    if (!--mpg123_initialised) {
        mpg123_exit();
    }
    return 0;
}

static int mp3_format_to_encoding(int encoding) {
    // https://www.mpg123.de/api/fmt123_8h_source.shtml
    // http://www.mega-nerd.com/libsndfile/api.html
    encoding++;
    return SF_FORMAT_MP3 | SF_FORMAT_PCM_16;
}

static int
mp3_read_header (SF_PRIVATE * psf, mpg123_handle * decoder) {
    int decoder_err, channels, encoding;
    long sample_rate;
    decoder_err = mpg123_getformat(decoder, &sample_rate, &channels, &encoding);
    if (decoder_err == MPG123_OK) {
        psf->sf.format = mp3_format_to_encoding(encoding);
        psf->sf.channels = channels;
        psf->sf.samplerate = sample_rate;
    }
    return decoder_err;
}

static ssize_t mp3_read_sf_handle (void * handle, void * buffer, size_t bytes) {
    SF_PRIVATE * psf = handle;
    off_t wasat = psf_fseek(psf, 0, SEEK_CUR);
    ssize_t rv = psf_fread(buffer, 1, bytes, psf);
    off_t nowat = psf_fseek(psf, 0, SEEK_CUR);
    printf("reading - wasat: %li rv: %li bytes: %lu nowat: %li\n", wasat, rv, bytes, nowat);
    return rv;
}

static off_t mp3_seek_sf_handle (void * handle, off_t offset, int whence) {
    SF_PRIVATE * psf = handle;
    off_t wasat = psf_fseek(psf, 0, SEEK_CUR);
    off_t rv = psf_fseek(psf, offset, whence);
    off_t nowat = psf_fseek(psf, 0, SEEK_CUR);
    char const * wname = NULL;
    switch (whence) {
        case SEEK_SET:
            wname = "SEEK_SET";
            break;
        case SEEK_CUR:
            wname = "SEEK_CUR";
            break;
        case SEEK_END:
            wname = "SEEK_END";
            break;
    }
    printf("seeking - wasat: %li rv: %li offset: %li whence: %s nowat: %li\n", wasat, rv, offset, wname, nowat);
    return rv;
}

static sf_count_t
mp3_read_2s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{
    static int const encoding = MPG123_ENC_SIGNED_16;
    size_t n_decoded = 0;
    mpg123_handle * decoder = psf->codec_data;
    int decoder_err = mpg123_format(decoder, psf->sf.samplerate, psf->sf.channels, encoding);
    if (decoder_err != MPG123_OK) {
        decoder_err = mpg123_read(
            decoder,
            (unsigned char *) ptr, len * psf->sf.channels * sizeof(short),
            &n_decoded);
    }
    return n_decoded;
}
