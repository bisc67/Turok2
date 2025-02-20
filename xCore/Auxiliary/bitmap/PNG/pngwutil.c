
/* pngwutil.c - utilities to write a PNG file
 *
 * libpng 1.0.5m - January 7, 2000
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, 1999, 2000 Glenn Randers-Pehrson
 */

#define PNG_INTERNAL
#include "png.h"

/* Place a 32-bit number into a buffer in PNG byte order.  We work
 * with unsigned numbers for convenience, although one supported
 * ancillary chunk uses signed (two's complement) numbers.
 */
void
xpng_save_uint_32(xpng_bytep buf, xpng_uint_32 i)
{
   buf[0] = (xpng_byte)((i >> 24) & 0xff);
   buf[1] = (xpng_byte)((i >> 16) & 0xff);
   buf[2] = (xpng_byte)((i >> 8) & 0xff);
   buf[3] = (xpng_byte)(i & 0xff);
}

#if defined(PNG_WRITE_pCAL_SUPPORTED)
/* The xpng_save_int_32 function assumes integers are stored in two's
 * complement format.  If this isn't the case, then this routine needs to
 * be modified to write data in two's complement format.
 */
void
xpng_save_int_32(xpng_bytep buf, xpng_int_32 i)
{
   buf[0] = (xpng_byte)((i >> 24) & 0xff);
   buf[1] = (xpng_byte)((i >> 16) & 0xff);
   buf[2] = (xpng_byte)((i >> 8) & 0xff);
   buf[3] = (xpng_byte)(i & 0xff);
}
#endif

/* Place a 16-bit number into a buffer in PNG byte order.
 * The parameter is declared unsigned int, not xpng_uint_16,
 * just to avoid potential problems on pre-ANSI C compilers.
 */
void
xpng_save_uint_16(xpng_bytep buf, unsigned int i)
{
   buf[0] = (xpng_byte)((i >> 8) & 0xff);
   buf[1] = (xpng_byte)(i & 0xff);
}

/* Write a PNG chunk all at once.  The type is an array of ASCII characters
 * representing the chunk name.  The array must be at least 4 bytes in
 * length, and does not need to be null terminated.  To be safe, pass the
 * pre-defined chunk names here, and if you need a new one, define it
 * where the others are defined.  The length is the length of the data.
 * All the data must be present.  If that is not possible, use the
 * xpng_write_chunk_start(), xpng_write_chunk_data(), and xpng_write_chunk_end()
 * functions instead.
 */
void
xpng_write_chunk(xpng_structp xpng_ptr, xpng_bytep chunk_name,
   xpng_bytep data, xpng_size_t length)
{
   xpng_write_chunk_start(xpng_ptr, chunk_name, (xpng_uint_32)length);
   xpng_write_chunk_data(xpng_ptr, data, length);
   xpng_write_chunk_end(xpng_ptr);
}

/* Write the start of a PNG chunk.  The type is the chunk type.
 * The total_length is the sum of the lengths of all the data you will be
 * passing in xpng_write_chunk_data().
 */
void
xpng_write_chunk_start(xpng_structp xpng_ptr, xpng_bytep chunk_name,
   xpng_uint_32 length)
{
   xpng_byte buf[4];
   xpng_debug2(0, "Writing %s chunk (%d bytes)\n", chunk_name, length);

   /* write the length */
   xpng_save_uint_32(buf, length);
   xpng_write_data(xpng_ptr, buf, (xpng_size_t)4);

   /* write the chunk name */
   xpng_write_data(xpng_ptr, chunk_name, (xpng_size_t)4);
   /* reset the crc and run it over the chunk name */
   xpng_reset_crc(xpng_ptr);
   xpng_calculate_crc(xpng_ptr, chunk_name, (xpng_size_t)4);
}

/* Write the data of a PNG chunk started with xpng_write_chunk_start().
 * Note that multiple calls to this function are allowed, and that the
 * sum of the lengths from these calls *must* add up to the total_length
 * given to xpng_write_chunk_start().
 */
void
xpng_write_chunk_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   /* write the data, and run the CRC over it */
   if (data != NULL && length > 0)
   {
      xpng_calculate_crc(xpng_ptr, data, length);
      xpng_write_data(xpng_ptr, data, length);
   }
}

/* Finish a chunk started with xpng_write_chunk_start(). */
void
xpng_write_chunk_end(xpng_structp xpng_ptr)
{
   xpng_byte buf[4];

   /* write the crc */
   xpng_save_uint_32(buf, xpng_ptr->crc);

   xpng_write_data(xpng_ptr, buf, (xpng_size_t)4);
}

/* Simple function to write the signature.  If we have already written
 * the magic bytes of the signature, or more likely, the PNG stream is
 * being embedded into another stream and doesn't need its own signature,
 * we should call xpng_set_sig_bytes() to tell libpng how many of the
 * bytes have already been written.
 */
void
xpng_write_sig(xpng_structp xpng_ptr)
{
   xpng_byte xpng_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   /* write the rest of the 8 byte signature */
   xpng_write_data(xpng_ptr, &xpng_signature[xpng_ptr->sig_bytes],
      (xpng_size_t)8 - xpng_ptr->sig_bytes);
}

#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_iCCP_SUPPORTED)
/*
 * This pair of functions encapsulates the operation of (a) compressing a
 * text string, and (b) issuing it later as a series of chunk data writes.
 * The compression_state structure is shared context for these functions
 * set up by the caller in order to make the whole mess thread-safe.
 */

typedef struct
{
    char *input;   /* the uncompressed input data */
    int input_len;   /* its length */
    int num_output_ptr; /* number of output pointers used */
    int max_output_ptr; /* size of output_ptr */
    xpng_charpp output_ptr; /* array of pointers to output */
} compression_state;

/* compress given text into storage in the xpng_ptr structure */
static int
xpng_text_compress(xpng_structp xpng_ptr,
        xpng_charp text, xpng_size_t text_len, int compression,
        compression_state *comp)
{
   int ret;

   comp->num_output_ptr = comp->max_output_ptr = 0;
   comp->output_ptr = NULL;
   comp->input = NULL;

   /* we may just want to pass the text right through */
   if (compression == PNG_TEXT_COMPRESSION_NONE)
   {
       comp->input = text;
       comp->input_len = text_len;
       return(text_len);
   }

   if (compression >= PNG_TEXT_COMPRESSION_LAST)
   {
#if !defined(PNG_NO_STDIO)
      char msg[50];
      sprintf(msg, "Unknown compression type %d", compression);
      xpng_warning(xpng_ptr, msg);
#else
      xpng_warning(xpng_ptr, "Unknown compression type");
#endif
      compression = PNG_TEXT_COMPRESSION_zTXt;
   }

   /* We can't write the chunk until we find out how much data we have,
    * which means we need to run the compressor first and save the
    * output.  This shouldn't be a problem, as the vast majority of
    * comments should be reasonable, but we will set up an array of
    * malloc'd pointers to be sure.
    *
    * If we knew the application was well behaved, we could simplify this
    * greatly by assuming we can always malloc an output buffer large
    * enough to hold the compressed text ((1001 * text_len / 1000) + 12)
    * and malloc this directly.  The only time this would be a bad idea is
    * if we can't malloc more than 64K and we have 64K of random input
    * data, or if the input string is incredibly large (although this
    * wouldn't cause a failure, just a slowdown due to swapping).
    */

   /* set up the compression buffers */
   xpng_ptr->zstream.avail_in = (uInt)text_len;
   xpng_ptr->zstream.next_in = (Bytef *)text;
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
   xpng_ptr->zstream.next_out = (Bytef *)xpng_ptr->zbuf;

   /* this is the same compression loop as in xpng_write_row() */
   do
   {
      /* compress the data */
      ret = deflate(&xpng_ptr->zstream, Z_NO_FLUSH);
      if (ret != Z_OK)
      {
         /* error */
         if (xpng_ptr->zstream.msg != NULL)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg);
         else
            xpng_error(xpng_ptr, "zlib error");
      }
      /* check to see if we need more room */
      if (!xpng_ptr->zstream.avail_out && xpng_ptr->zstream.avail_in)
      {
         /* make sure the output array has room */
         if (comp->num_output_ptr >= comp->max_output_ptr)
         {
            int old_max;

            old_max = comp->max_output_ptr;
            comp->max_output_ptr = comp->num_output_ptr + 4;
            if (comp->output_ptr != NULL)
            {
               xpng_charpp old_ptr;

               old_ptr = comp->output_ptr;
               comp->output_ptr = (xpng_charpp)xpng_malloc(xpng_ptr,
                  (xpng_uint_32)(comp->max_output_ptr * sizeof (xpng_charpp)));
               xpng_memcpy(comp->output_ptr, old_ptr,
           old_max * sizeof (xpng_charp));
               xpng_free(xpng_ptr, old_ptr);
            }
            else
               comp->output_ptr = (xpng_charpp)xpng_malloc(xpng_ptr,
                  (xpng_uint_32)(comp->max_output_ptr * sizeof (xpng_charp)));
         }

         /* save the data */
         comp->output_ptr[comp->num_output_ptr] = (xpng_charp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)xpng_ptr->zbuf_size);
         xpng_memcpy(comp->output_ptr[comp->num_output_ptr], xpng_ptr->zbuf,
            xpng_ptr->zbuf_size);
         comp->num_output_ptr++;

         /* and reset the buffer */
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
         xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
      }
   /* continue until we don't have any more to compress */
   } while (xpng_ptr->zstream.avail_in);

   /* finish the compression */
   do
   {
      /* tell zlib we are finished */
      ret = deflate(&xpng_ptr->zstream, Z_FINISH);
      if (ret != Z_OK && ret != Z_STREAM_END)
      {
         /* we got an error */
         if (xpng_ptr->zstream.msg != NULL)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg);
         else
            xpng_error(xpng_ptr, "zlib error");
      }

      /* check to see if we need more room */
      if (!(xpng_ptr->zstream.avail_out) && ret == Z_OK)
      {
         /* check to make sure our output array has room */
         if (comp->num_output_ptr >= comp->max_output_ptr)
         {
            int old_max;

            old_max = comp->max_output_ptr;
            comp->max_output_ptr = comp->num_output_ptr + 4;
            if (comp->output_ptr != NULL)
            {
               xpng_charpp old_ptr;

               old_ptr = comp->output_ptr;
               /* This could be optimized to realloc() */
               comp->output_ptr = (xpng_charpp)xpng_malloc(xpng_ptr,
                  (xpng_uint_32)(comp->max_output_ptr * sizeof (xpng_charpp)));
               xpng_memcpy(comp->output_ptr, old_ptr,
           old_max * sizeof (xpng_charp));
               xpng_free(xpng_ptr, old_ptr);
            }
            else
               comp->output_ptr = (xpng_charpp)xpng_malloc(xpng_ptr,
                  (xpng_uint_32)(comp->max_output_ptr * sizeof (xpng_charp)));
         }

         /* save off the data */
         comp->output_ptr[comp->num_output_ptr] = (xpng_charp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)xpng_ptr->zbuf_size);
         xpng_memcpy(comp->output_ptr[comp->num_output_ptr], xpng_ptr->zbuf,
            xpng_ptr->zbuf_size);
         comp->num_output_ptr++;

         /* and reset the buffer pointers */
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
         xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
      }
   } while (ret != Z_STREAM_END);

   /* text length is number of buffers plus last buffer */
   text_len = xpng_ptr->zbuf_size * comp->num_output_ptr;
   if (xpng_ptr->zstream.avail_out < xpng_ptr->zbuf_size)
      text_len += xpng_ptr->zbuf_size - (xpng_size_t)xpng_ptr->zstream.avail_out;

   return(text_len);
}

/* ship the compressed text out via chunk writes */
static void
xpng_write_compressed_data_out(xpng_structp xpng_ptr, compression_state *comp)
{
   int i;

   /* handle the no-compression case */
   if (comp->input)
   {
       xpng_write_chunk_data(xpng_ptr, (xpng_bytep)comp->input, comp->input_len);
       return;
   }

   /* write saved output buffers, if any */
   for (i = 0; i < comp->num_output_ptr; i++)
   {
      xpng_write_chunk_data(xpng_ptr,(xpng_bytep)comp->output_ptr[i],
         xpng_ptr->zbuf_size);
      xpng_free(xpng_ptr, comp->output_ptr[i]);
   }
   if (comp->max_output_ptr != 0)
      xpng_free(xpng_ptr, comp->output_ptr);
   /* write anything left in zbuf */
   if (xpng_ptr->zstream.avail_out < (xpng_uint_32)xpng_ptr->zbuf_size)
      xpng_write_chunk_data(xpng_ptr, xpng_ptr->zbuf,
         xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out);

   /* reset zlib for another zTXt/iTXt or the image data */
   deflateReset(&xpng_ptr->zstream);

}
#endif

/* Write the IHDR chunk, and update the xpng_struct with the necessary
 * information.  Note that the rest of this code depends upon this
 * information being correct.
 */
void
xpng_write_IHDR(xpng_structp xpng_ptr, xpng_uint_32 width, xpng_uint_32 height,
   int bit_depth, int color_type, int compression_type, int filter_type,
   int interlace_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_IHDR;
#endif
   xpng_byte buf[13]; /* buffer to store the IHDR info */

   xpng_debug(1, "in xpng_write_IHDR\n");
   /* Check that we have valid input data from the application info */
   switch (color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16: xpng_ptr->channels = 1; break;
            default: xpng_error(xpng_ptr,"Invalid bit depth for grayscale image");
         }
         break;
      case PNG_COLOR_TYPE_RGB:
         if (bit_depth != 8 && bit_depth != 16)
            xpng_error(xpng_ptr, "Invalid bit depth for RGB image");
         xpng_ptr->channels = 3;
         break;
      case PNG_COLOR_TYPE_PALETTE:
         switch (bit_depth)
         {
            case 1:
            case 2:
            case 4:
            case 8: xpng_ptr->channels = 1; break;
            default: xpng_error(xpng_ptr, "Invalid bit depth for paletted image");
         }
         break;
      case PNG_COLOR_TYPE_GRAY_ALPHA:
         if (bit_depth != 8 && bit_depth != 16)
            xpng_error(xpng_ptr, "Invalid bit depth for grayscale+alpha image");
         xpng_ptr->channels = 2;
         break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
         if (bit_depth != 8 && bit_depth != 16)
            xpng_error(xpng_ptr, "Invalid bit depth for RGBA image");
         xpng_ptr->channels = 4;
         break;
      default:
         xpng_error(xpng_ptr, "Invalid image color type specified");
   }

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
   {
      xpng_warning(xpng_ptr, "Invalid compression type specified");
      compression_type = PNG_COMPRESSION_TYPE_BASE;
   }

   if (filter_type != PNG_FILTER_TYPE_BASE)
   {
      xpng_warning(xpng_ptr, "Invalid filter type specified");
      filter_type = PNG_FILTER_TYPE_BASE;
   }

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   if (interlace_type != PNG_INTERLACE_NONE &&
      interlace_type != PNG_INTERLACE_ADAM7)
   {
      xpng_warning(xpng_ptr, "Invalid interlace type specified");
      interlace_type = PNG_INTERLACE_ADAM7;
   }
#else
   interlace_type=PNG_INTERLACE_NONE;
#endif

   /* save off the relevent information */
   xpng_ptr->bit_depth = (xpng_byte)bit_depth;
   xpng_ptr->color_type = (xpng_byte)color_type;
   xpng_ptr->interlaced = (xpng_byte)interlace_type;
   xpng_ptr->width = width;
   xpng_ptr->height = height;

   xpng_ptr->pixel_depth = (xpng_byte)(bit_depth * xpng_ptr->channels);
   xpng_ptr->rowbytes = ((width * (xpng_size_t)xpng_ptr->pixel_depth + 7) >> 3);
   /* set the usr info, so any transformations can modify it */
   xpng_ptr->usr_width = xpng_ptr->width;
   xpng_ptr->usr_bit_depth = xpng_ptr->bit_depth;
   xpng_ptr->usr_channels = xpng_ptr->channels;

   /* pack the header information into the buffer */
   xpng_save_uint_32(buf, width);
   xpng_save_uint_32(buf + 4, height);
   buf[8] = (xpng_byte)bit_depth;
   buf[9] = (xpng_byte)color_type;
   buf[10] = (xpng_byte)compression_type;
   buf[11] = (xpng_byte)filter_type;
   buf[12] = (xpng_byte)interlace_type;

   /* write the chunk */
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_IHDR, buf, (xpng_size_t)13);

   /* initialize zlib with PNG info */
   xpng_ptr->zstream.zalloc = xpng_zalloc;
   xpng_ptr->zstream.zfree = xpng_zfree;
   xpng_ptr->zstream.opaque = (voidpf)xpng_ptr;
   if (!(xpng_ptr->do_filter))
   {
      if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE ||
         xpng_ptr->bit_depth < 8)
         xpng_ptr->do_filter = PNG_FILTER_NONE;
      else
         xpng_ptr->do_filter = PNG_ALL_FILTERS;
   }
   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_STRATEGY))
   {
      if (xpng_ptr->do_filter != PNG_FILTER_NONE)
         xpng_ptr->zlib_strategy = Z_FILTERED;
      else
         xpng_ptr->zlib_strategy = Z_DEFAULT_STRATEGY;
   }
   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_LEVEL))
      xpng_ptr->zlib_level = Z_DEFAULT_COMPRESSION;
   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL))
      xpng_ptr->zlib_mem_level = 8;
   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS))
      xpng_ptr->zlib_window_bits = 15;
   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_CUSTOM_METHOD))
      xpng_ptr->zlib_method = 8;
   deflateInit2(&xpng_ptr->zstream, xpng_ptr->zlib_level,
      xpng_ptr->zlib_method, xpng_ptr->zlib_window_bits,
      xpng_ptr->zlib_mem_level, xpng_ptr->zlib_strategy);
   xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;

   xpng_ptr->mode = PNG_HAVE_IHDR;
}

/* write the palette.  We are careful not to trust xpng_color to be in the
 * correct order for PNG, so people can redefine it to any convenient
 * structure.
 */
void
xpng_write_PLTE(xpng_structp xpng_ptr, xpng_colorp palette, xpng_uint_32 num_pal)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_PLTE;
#endif
   xpng_uint_32 i;
   xpng_colorp pal_ptr;
   xpng_byte buf[3];

   xpng_debug(1, "in xpng_write_PLTE\n");
   if ((
#ifdef PNG_WRITE_EMPTY_PLTE_SUPPORTED
        !xpng_ptr->empty_plte_permitted &&
#endif
        num_pal == 0) || num_pal > 256)
     {
       if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
         {
           xpng_error(xpng_ptr, "Invalid number of colors in palette");
         }
       else
         {
           xpng_warning(xpng_ptr, "Invalid number of colors in palette");
           return;
         }
   }

   xpng_ptr->num_palette = (xpng_uint_16)num_pal;
   xpng_debug1(3, "num_palette = %d\n", xpng_ptr->num_palette);

   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_PLTE, num_pal * 3);
   for (i = 0, pal_ptr = palette; i < num_pal; i++, pal_ptr++)
   {
      buf[0] = pal_ptr->red;
      buf[1] = pal_ptr->green;
      buf[2] = pal_ptr->blue;
      xpng_write_chunk_data(xpng_ptr, buf, (xpng_size_t)3);
   }
   xpng_write_chunk_end(xpng_ptr);
   xpng_ptr->mode |= PNG_HAVE_PLTE;
}

/* write an IDAT chunk */
void
xpng_write_IDAT(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_IDAT;
#endif
   xpng_debug(1, "in xpng_write_IDAT\n");
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_IDAT, data, length);
   xpng_ptr->mode |= PNG_HAVE_IDAT;
}

/* write an IEND chunk */
void
xpng_write_IEND(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_IEND;
#endif
   xpng_debug(1, "in xpng_write_IEND\n");
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_IEND, NULL, (xpng_size_t)0);
   xpng_ptr->mode |= PNG_HAVE_IEND;
}

#if defined(PNG_WRITE_gAMA_SUPPORTED)
/* write a gAMA chunk */
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_write_gAMA(xpng_structp xpng_ptr, double file_gamma)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_gAMA;
#endif
   xpng_uint_32 igamma;
   xpng_byte buf[4];

   xpng_debug(1, "in xpng_write_gAMA\n");
   /* file_gamma is saved in 1/100,000ths */
   igamma = (xpng_uint_32)(file_gamma * 100000.0 + 0.5);
   xpng_save_uint_32(buf, igamma);
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_gAMA, buf, (xpng_size_t)4);
}
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
void
xpng_write_gAMA_fixed(xpng_structp xpng_ptr, xpng_fixed_point file_gamma)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_gAMA;
#endif
   xpng_byte buf[4];

   xpng_debug(1, "in xpng_write_gAMA\n");
   /* file_gamma is saved in 1/100,000ths */
   xpng_save_uint_32(buf, file_gamma);
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_gAMA, buf, (xpng_size_t)4);
}
#endif
#endif

#if defined(PNG_WRITE_sRGB_SUPPORTED)
/* write a sRGB chunk */
void
xpng_write_sRGB(xpng_structp xpng_ptr, int srgb_intent)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_sRGB;
#endif
   xpng_byte buf[1];

   xpng_debug(1, "in xpng_write_sRGB\n");
   if(srgb_intent >= PNG_sRGB_INTENT_LAST)
         xpng_warning(xpng_ptr,
            "Invalid sRGB rendering intent specified");
   buf[0]=(xpng_byte)srgb_intent;
   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_sRGB, buf, (xpng_size_t)1);
}
#endif

#if defined(PNG_WRITE_iCCP_SUPPORTED)
/* write an iCCP chunk */
void
xpng_write_iCCP(xpng_structp xpng_ptr, xpng_charp name, int compression_type,
   xpng_charp profile, int profile_len)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_iCCP;
#endif
   xpng_size_t name_len;
   xpng_charp new_name;
   compression_state comp;

   xpng_debug(1, "in xpng_write_iCCP\n");
   if (name == NULL || (name_len = xpng_check_keyword(xpng_ptr, name,
      &new_name)) == 0)
   {
      xpng_warning(xpng_ptr, "Empty keyword in iCCP chunk");
      return;
   }

   if (compression_type)
      /* ignore */ ;

   if (profile == NULL || *profile == '\0')
      profile_len = 0;

   if (profile_len)
       profile_len = xpng_text_compress(xpng_ptr, profile, profile_len,
                   PNG_TEXT_COMPRESSION_zTXt, &comp);

   /* make sure we include the NULL after the name and the compression type */
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_iCCP,
          (xpng_uint_32)name_len+profile_len+2);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_name, name_len + 2);

   if (profile_len)
      xpng_write_compressed_data_out(xpng_ptr, &comp);

   xpng_write_chunk_end(xpng_ptr);
   xpng_free(xpng_ptr, new_name);
}
#endif

#if defined(PNG_WRITE_sPLT_SUPPORTED)
/* write a sPLT chunk */
void
xpng_write_sPLT(xpng_structp xpng_ptr, xpng_spalette_p spalette)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_sPLT;
#endif
   xpng_size_t name_len;
   xpng_charp new_name;
   xpng_byte entrybuf[10];
   int entry_size = (spalette->depth == 8 ? 6 : 10);
   int palette_size = entry_size * spalette->nentries;
   xpng_spalette_entryp ep;

   xpng_debug(1, "in xpng_write_sPLT\n");
   if (spalette->name == NULL || (name_len = xpng_check_keyword(xpng_ptr, spalette->name, &new_name))==0)
   {
      xpng_warning(xpng_ptr, "Empty keyword in sPLT chunk");
      return;
   }

   /* make sure we include the NULL after the name */
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep) xpng_sPLT,
          (xpng_uint_32)(name_len + 2 + palette_size));
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_name, name_len + 1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)&spalette->depth, 1);

   /* loop through each palette entry, writing appropriately */
   for (ep = spalette->entries; ep<spalette->entries+spalette->nentries; ep++)
   {
       if (spalette->depth == 8)
       {
           entrybuf[0] = (xpng_byte)ep->red;
           entrybuf[1] = (xpng_byte)ep->green;
           entrybuf[2] = (xpng_byte)ep->blue;
           entrybuf[3] = (xpng_byte)ep->alpha;
           xpng_save_uint_16(entrybuf + 4, ep->frequency);
       }
       else
       {
           xpng_save_uint_16(entrybuf + 0, ep->red);
           xpng_save_uint_16(entrybuf + 2, ep->green);
           xpng_save_uint_16(entrybuf + 4, ep->blue);
           xpng_save_uint_16(entrybuf + 6, ep->alpha);
           xpng_save_uint_16(entrybuf + 8, ep->frequency);
       }
       xpng_write_chunk_data(xpng_ptr, entrybuf, entry_size);
   }

   xpng_write_chunk_end(xpng_ptr);
   xpng_free(xpng_ptr, new_name);
}
#endif

#if defined(PNG_WRITE_sBIT_SUPPORTED)
/* write the sBIT chunk */
void
xpng_write_sBIT(xpng_structp xpng_ptr, xpng_color_8p sbit, int color_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_sBIT;
#endif
   xpng_byte buf[4];
   xpng_size_t size;

   xpng_debug(1, "in xpng_write_sBIT\n");
   /* make sure we don't depend upon the order of PNG_COLOR_8 */
   if (color_type & PNG_COLOR_MASK_COLOR)
   {
      xpng_byte maxbits;

      maxbits = (xpng_byte)(color_type==PNG_COLOR_TYPE_PALETTE ? 8 :
                xpng_ptr->usr_bit_depth);
      if (sbit->red == 0 || sbit->red > maxbits ||
          sbit->green == 0 || sbit->green > maxbits ||
          sbit->blue == 0 || sbit->blue > maxbits)
      {
         xpng_warning(xpng_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[0] = sbit->red;
      buf[1] = sbit->green;
      buf[2] = sbit->blue;
      size = 3;
   }
   else
   {
      if (sbit->gray == 0 || sbit->gray > xpng_ptr->usr_bit_depth)
      {
         xpng_warning(xpng_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[0] = sbit->gray;
      size = 1;
   }

   if (color_type & PNG_COLOR_MASK_ALPHA)
   {
      if (sbit->alpha == 0 || sbit->alpha > xpng_ptr->usr_bit_depth)
      {
         xpng_warning(xpng_ptr, "Invalid sBIT depth specified");
         return;
      }
      buf[size++] = sbit->alpha;
   }

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_sBIT, buf, size);
}
#endif

#if defined(PNG_WRITE_cHRM_SUPPORTED)
/* write the cHRM chunk */
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_write_cHRM(xpng_structp xpng_ptr, double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_cHRM;
#endif
   xpng_byte buf[32];
   xpng_uint_32 itemp;

   xpng_debug(1, "in xpng_write_cHRM\n");
   /* each value is saved in 1/100,000ths */
   if (white_x < 0 || white_x > 0.8 || white_y < 0 || white_y > 0.8 ||
       white_x + white_y > 1.0)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM white point specified");
      printf("white_x=%f, white_y=%f\n",white_x, white_y);
      return;
   }
   itemp = (xpng_uint_32)(white_x * 100000.0 + 0.5);
   xpng_save_uint_32(buf, itemp);
   itemp = (xpng_uint_32)(white_y * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 4, itemp);

   if (red_x < 0 || red_x > 0.8 || red_y < 0 || red_y > 0.8 ||
       red_x + red_y > 1.0)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM red point specified");
      return;
   }
   itemp = (xpng_uint_32)(red_x * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 8, itemp);
   itemp = (xpng_uint_32)(red_y * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 12, itemp);

   if (green_x < 0 || green_x > 0.8 || green_y < 0 || green_y > 0.8 ||
       green_x + green_y > 1.0)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM green point specified");
      return;
   }
   itemp = (xpng_uint_32)(green_x * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 16, itemp);
   itemp = (xpng_uint_32)(green_y * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 20, itemp);

   if (blue_x < 0 || blue_x > 0.8 || blue_y < 0 || blue_y > 0.8 ||
       blue_x + blue_y > 1.0)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM blue point specified");
      return;
   }
   itemp = (xpng_uint_32)(blue_x * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 24, itemp);
   itemp = (xpng_uint_32)(blue_y * 100000.0 + 0.5);
   xpng_save_uint_32(buf + 28, itemp);

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_cHRM, buf, (xpng_size_t)32);
}
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
void
xpng_write_cHRM_fixed(xpng_structp xpng_ptr, xpng_fixed_point white_x,
   xpng_fixed_point white_y, xpng_fixed_point red_x, xpng_fixed_point red_y,
   xpng_fixed_point green_x, xpng_fixed_point green_y, xpng_fixed_point blue_x,
   xpng_fixed_point blue_y)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_cHRM;
#endif
   xpng_byte buf[32];

   xpng_debug(1, "in xpng_write_cHRM\n");
   /* each value is saved in 1/100,000ths */
   if (white_x > 80000L || white_y > 80000L || white_x + white_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid fixed cHRM white point specified");
      printf("white_x=%d, white_y=%d\n",white_x, white_y);
      return;
   }
   xpng_save_uint_32(buf, white_x);
   xpng_save_uint_32(buf + 4, white_y);

   if (red_x > 80000L || red_y > 80000L || red_x + red_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM fixed red point specified");
      return;
   }
   xpng_save_uint_32(buf + 8, red_x);
   xpng_save_uint_32(buf + 12, red_y);

   if (green_x > 80000L || green_y > 80000L || green_x + green_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid fixed cHRM green point specified");
      return;
   }
   xpng_save_uint_32(buf + 16, green_x);
   xpng_save_uint_32(buf + 20, green_y);

   if (blue_x > 80000L || blue_y > 80000L || blue_x + blue_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid fixed cHRM blue point specified");
      return;
   }
   xpng_save_uint_32(buf + 24, blue_x);
   xpng_save_uint_32(buf + 28, blue_y);

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_cHRM, buf, (xpng_size_t)32);
}
#endif
#endif

#if defined(PNG_WRITE_tRNS_SUPPORTED)
/* write the tRNS chunk */
void
xpng_write_tRNS(xpng_structp xpng_ptr, xpng_bytep trans, xpng_color_16p tran,
   int num_trans, int color_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_tRNS;
#endif
   xpng_byte buf[6];

   xpng_debug(1, "in xpng_write_tRNS\n");
   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (num_trans <= 0 || num_trans > (int)xpng_ptr->num_palette)
      {
         xpng_warning(xpng_ptr,"Invalid number of transparent colors specified");
         return;
      }
      /* write the chunk out as it is */
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_tRNS, trans, (xpng_size_t)num_trans);
   }
   else if (color_type == PNG_COLOR_TYPE_GRAY)
   {
      /* one 16 bit value */
      xpng_save_uint_16(buf, tran->gray);
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_tRNS, buf, (xpng_size_t)2);
   }
   else if (color_type == PNG_COLOR_TYPE_RGB)
   {
      /* three 16 bit values */
      xpng_save_uint_16(buf, tran->red);
      xpng_save_uint_16(buf + 2, tran->green);
      xpng_save_uint_16(buf + 4, tran->blue);
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_tRNS, buf, (xpng_size_t)6);
   }
   else
   {
      xpng_warning(xpng_ptr, "Can't write tRNS with an alpha channel");
   }
}
#endif

#if defined(PNG_WRITE_bKGD_SUPPORTED)
/* write the background chunk */
void
xpng_write_bKGD(xpng_structp xpng_ptr, xpng_color_16p back, int color_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_bKGD;
#endif
   xpng_byte buf[6];

   xpng_debug(1, "in xpng_write_bKGD\n");
   if (color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (
#ifdef PNG_WRITE_EMPTY_PLTE_SUPPORTED
          (!xpng_ptr->empty_plte_permitted ||
          (xpng_ptr->empty_plte_permitted && xpng_ptr->num_palette)) &&
#endif
         back->index > xpng_ptr->num_palette)
      {
         xpng_warning(xpng_ptr, "Invalid background palette index");
         return;
      }
      buf[0] = back->index;
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_bKGD, buf, (xpng_size_t)1);
   }
   else if (color_type & PNG_COLOR_MASK_COLOR)
   {
      xpng_save_uint_16(buf, back->red);
      xpng_save_uint_16(buf + 2, back->green);
      xpng_save_uint_16(buf + 4, back->blue);
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_bKGD, buf, (xpng_size_t)6);
   }
   else
   {
      xpng_save_uint_16(buf, back->gray);
      xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_bKGD, buf, (xpng_size_t)2);
   }
}
#endif

#if defined(PNG_WRITE_hIST_SUPPORTED)
/* write the histogram */
void
xpng_write_hIST(xpng_structp xpng_ptr, xpng_uint_16p hist, int num_hist)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_hIST;
#endif
   int i;
   xpng_byte buf[3];

   xpng_debug(1, "in xpng_write_hIST\n");
   if (num_hist > (int)xpng_ptr->num_palette)
   {
      xpng_debug2(3, "num_hist = %d, num_palette = %d\n", num_hist,
         xpng_ptr->num_palette);
      xpng_warning(xpng_ptr, "Invalid number of histogram entries specified");
      return;
   }

   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_hIST, (xpng_uint_32)(num_hist * 2));
   for (i = 0; i < num_hist; i++)
   {
      xpng_save_uint_16(buf, hist[i]);
      xpng_write_chunk_data(xpng_ptr, buf, (xpng_size_t)2);
   }
   xpng_write_chunk_end(xpng_ptr);
}
#endif

#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED) || \
    defined(PNG_WRITE_iCCP_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
/* Check that the tEXt or zTXt keyword is valid per PNG 1.0 specification,
 * and if invalid, correct the keyword rather than discarding the entire
 * chunk.  The PNG 1.0 specification requires keywords 1-79 characters in
 * length, forbids leading or trailing whitespace, multiple internal spaces,
 * and the non-break space (0x80) from ISO 8859-1.  Returns keyword length.
 *
 * The new_key is allocated to hold the corrected keyword and must be freed
 * by the calling routine.  This avoids problems with trying to write to
 * static keywords without having to have duplicate copies of the strings.
 */
xpng_size_t
xpng_check_keyword(xpng_structp xpng_ptr, xpng_charp key, xpng_charpp new_key)
{
   xpng_size_t key_len;
   xpng_charp kp, dp;
   int kflag;

   xpng_debug(1, "in xpng_check_keyword\n");
   *new_key = NULL;

   if (key == NULL || (key_len = xpng_strlen(key)) == 0)
   {
      xpng_chunk_warning(xpng_ptr, "zero length keyword");
      return ((xpng_size_t)0);
   }

   xpng_debug1(2, "Keyword to be checked is '%s'\n", key);

   *new_key = (xpng_charp)xpng_malloc(xpng_ptr, (xpng_uint_32)(key_len + 1));

   /* Replace non-printing characters with a blank and print a warning */
   for (kp = key, dp = *new_key; *kp != '\0'; kp++, dp++)
   {
      if (*kp < 0x20 || (*kp > 0x7E && (xpng_byte)*kp < 0xA1))
      {
#if !defined(PNG_NO_STDIO)
         char msg[40];

         sprintf(msg, "invalid keyword character 0x%02X", *kp);
         xpng_chunk_warning(xpng_ptr, msg);
#else
         xpng_chunk_warning(xpng_ptr, "invalid character in keyword");
#endif
         *dp = ' ';
      }
      else
      {
         *dp = *kp;
      }
   }
   *dp = '\0';

   /* Remove any trailing white space. */
   kp = *new_key + key_len - 1;
   if (*kp == ' ')
   {
      xpng_chunk_warning(xpng_ptr, "trailing spaces removed from keyword");

      while (*kp == ' ')
      {
        *(kp--) = '\0';
        key_len--;
      }
   }

   /* Remove any leading white space. */
   kp = *new_key;
   if (*kp == ' ')
   {
      xpng_chunk_warning(xpng_ptr, "leading spaces removed from keyword");

      while (*kp == ' ')
      {
        kp++;
        key_len--;
      }
   }

   xpng_debug1(2, "Checking for multiple internal spaces in '%s'\n", kp);

   /* Remove multiple internal spaces. */
   for (kflag = 0, dp = *new_key; *kp != '\0'; kp++)
   {
      if (*kp == ' ' && kflag == 0)
      {
         *(dp++) = *kp;
         kflag = 1;
      }
      else if (*kp == ' ')
      {
         key_len--;
      }
      else
      {
         *(dp++) = *kp;
         kflag = 0;
      }
   }
   *dp = '\0';

   if (key_len == 0)
   {
      xpng_free(xpng_ptr, *new_key);
      *new_key=NULL;
      xpng_chunk_warning(xpng_ptr, "Zero length keyword");
   }

   if (key_len > 79)
   {
      xpng_chunk_warning(xpng_ptr, "keyword length must be 1 - 79 characters");
      new_key[79] = '\0';
      key_len = 79;
   }

   return (key_len);
}
#endif

#if defined(PNG_WRITE_tEXt_SUPPORTED)
/* write a tEXt chunk */
void
xpng_write_tEXt(xpng_structp xpng_ptr, xpng_charp key, xpng_charp text,
   xpng_size_t text_len)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_tEXt;
#endif
   xpng_size_t key_len;
   xpng_charp new_key;

   xpng_debug(1, "in xpng_write_tEXt\n");
   if (key == NULL || (key_len = xpng_check_keyword(xpng_ptr, key, &new_key))==0)
   {
      xpng_warning(xpng_ptr, "Empty keyword in tEXt chunk");
      return;
   }

   if (text == NULL || *text == '\0')
      text_len = 0;
   else
      text_len = xpng_strlen(text);

   /* make sure we include the 0 after the key */
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_tEXt, (xpng_uint_32)key_len+text_len+1);
   /*
    * We leave it to the application to meet PNG-1.0 requirements on the
    * contents of the text.  PNG-1.0 through PNG-1.2 discourage the use of
    * any non-Latin-1 characters except for NEWLINE.  ISO PNG will forbid them.
    * The NUL character is forbidden by PNG-1.0 through PNG-1.2 and ISO PNG.
    */
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_key, key_len + 1);
   if (text_len)
      xpng_write_chunk_data(xpng_ptr, (xpng_bytep)text, text_len);

   xpng_write_chunk_end(xpng_ptr);
   xpng_free(xpng_ptr, new_key);
}
#endif

#if defined(PNG_WRITE_zTXt_SUPPORTED)
/* write a compressed text chunk */
void
xpng_write_zTXt(xpng_structp xpng_ptr, xpng_charp key, xpng_charp text,
   xpng_size_t text_len, int compression)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_zTXt;
#endif
   xpng_size_t key_len;
   char buf[1];
   xpng_charp new_key;
   compression_state comp;

   xpng_debug(1, "in xpng_write_zTXt\n");

   if (key == NULL || (key_len = xpng_check_keyword(xpng_ptr, key, &new_key))==0)
   {
      xpng_warning(xpng_ptr, "Empty keyword in zTXt chunk");
      return;
   }

   if (text == NULL || *text == '\0' || compression==PNG_TEXT_COMPRESSION_NONE)
   {
      xpng_write_tEXt(xpng_ptr, new_key, text, (xpng_size_t)0);
      xpng_free(xpng_ptr, new_key);
      return;
   }
   
   text_len = xpng_strlen(text);

   xpng_free(xpng_ptr, new_key);

   /* compute the compressed data; do it now for the length */
   text_len = xpng_text_compress(xpng_ptr, text, text_len, compression, &comp);

   /* write start of chunk */
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_zTXt, (xpng_uint_32)
      (key_len+text_len+2));
   /* write key */
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)key, key_len + 1);
   buf[0] = (xpng_byte)compression;
   /* write compression */
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)buf, (xpng_size_t)1);
   /* write the compressed data */
   xpng_write_compressed_data_out(xpng_ptr, &comp);

   /* close the chunk */
   xpng_write_chunk_end(xpng_ptr);
}
#endif

#if defined(PNG_WRITE_iTXt_SUPPORTED)
/* write an iTXt chunk */
void
xpng_write_iTXt(xpng_structp xpng_ptr, int compression, xpng_charp key,
    xpng_charp lang, xpng_charp lang_key, xpng_charp text)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_iTXt;
#endif
   xpng_size_t lang_len, key_len, lang_key_len, text_len;
   xpng_charp new_lang, new_key;
   xpng_byte cbuf[2];
   compression_state comp;

   xpng_debug(1, "in xpng_write_iTXt\n");

   if (key == NULL || (key_len = xpng_check_keyword(xpng_ptr, key, &new_key))==0)
   {
      xpng_warning(xpng_ptr, "Empty keyword in iTXt chunk");
      return;
   }
   if (lang == NULL || (lang_len = xpng_check_keyword(xpng_ptr, lang,
      &new_lang))==0)
   {
      xpng_warning(xpng_ptr, "Empty language field in iTXt chunk");
      return;
   }
   lang_key_len = xpng_strlen(lang_key);
   text_len = xpng_strlen(text);

   if (text == NULL || *text == '\0')
      text_len = 0;

   /* compute the compressed data; do it now for the length */
   text_len = xpng_text_compress(xpng_ptr, text, text_len, compression-2, &comp);

   /* make sure we include the compression flag, the compression byte,
    * and the NULs after the key, lang, and lang_key parts */

   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_iTXt,
          (xpng_uint_32)(
        5 /* comp byte, comp flag, terminators for key, lang and lang_key */
        + key_len
        + lang_len
        + lang_key_len
        + text_len));

   /*
    * We leave it to the application to meet PNG-1.0 requirements on the
    * contents of the text.  PNG-1.0 through PNG-1.2 discourage the use of
    * any non-Latin-1 characters except for NEWLINE.  ISO PNG will forbid them.
    * The NUL character is forbidden by PNG-1.0 through PNG-1.2 and ISO PNG.
    */
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_key, key_len + 1);

   /* set the compression flag */
   if (compression == PNG_ITXT_COMPRESSION_NONE || \
       compression == PNG_TEXT_COMPRESSION_NONE)
       cbuf[0] = 0;
   else /* compression == PNG_ITXT_COMPRESSION_zTXt */
       cbuf[0] = 1;
   /* set the compression method */
   cbuf[1] = 0;
   xpng_write_chunk_data(xpng_ptr, cbuf, 2);

   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_lang, lang_len + 1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)lang_key, lang_key_len+1);
   xpng_write_chunk_data(xpng_ptr, '\0', 1);

   xpng_write_compressed_data_out(xpng_ptr, &comp);

   xpng_write_chunk_end(xpng_ptr);
   xpng_free(xpng_ptr, new_key);
   xpng_free(xpng_ptr, new_lang);
}
#endif

#if defined(PNG_WRITE_oFFs_SUPPORTED)
/* write the oFFs chunk */
void
xpng_write_oFFs(xpng_structp xpng_ptr, xpng_uint_32 x_offset,
   xpng_uint_32 y_offset,
   int unit_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_oFFs;
#endif
   xpng_byte buf[9];

   xpng_debug(1, "in xpng_write_oFFs\n");
   if (unit_type >= PNG_OFFSET_LAST)
      xpng_warning(xpng_ptr, "Unrecognized unit type for oFFs chunk");

   xpng_save_uint_32(buf, x_offset);
   xpng_save_uint_32(buf + 4, y_offset);
   buf[8] = (xpng_byte)unit_type;

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_oFFs, buf, (xpng_size_t)9);
}
#endif

#if defined(PNG_WRITE_pCAL_SUPPORTED)
/* write the pCAL chunk (png-scivis-19970203) */
void
xpng_write_pCAL(xpng_structp xpng_ptr, xpng_charp purpose, xpng_int_32 X0,
   xpng_int_32 X1, int type, int nparams, xpng_charp units, xpng_charpp params)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_pCAL;
#endif
   xpng_size_t purpose_len, units_len, total_len;
   xpng_uint_32p params_len;
   xpng_byte buf[10];
   xpng_charp new_purpose;
   int i;

   xpng_debug1(1, "in xpng_write_pCAL (%d parameters)\n", nparams);
   if (type >= PNG_EQUATION_LAST)
      xpng_warning(xpng_ptr, "Unrecognized equation type for pCAL chunk");

   purpose_len = xpng_check_keyword(xpng_ptr, purpose, &new_purpose) + 1;
   xpng_debug1(3, "pCAL purpose length = %d\n", purpose_len);
   units_len = xpng_strlen(units) + (nparams == 0 ? 0 : 1);
   xpng_debug1(3, "pCAL units length = %d\n", units_len);
   total_len = purpose_len + units_len + 10;

   params_len = (xpng_uint_32p)xpng_malloc(xpng_ptr, (xpng_uint_32)(nparams
      *sizeof(xpng_uint_32)));

   /* Find the length of each parameter, making sure we don't count the
      null terminator for the last parameter. */
   for (i = 0; i < nparams; i++)
   {
      params_len[i] = xpng_strlen(params[i]) + (i == nparams - 1 ? 0 : 1);
      xpng_debug2(3, "pCAL parameter %d length = %d\n", i, params_len[i]);
      total_len += (xpng_size_t)params_len[i];
   }

   xpng_debug1(3, "pCAL total length = %d\n", total_len);
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_pCAL, (xpng_uint_32)total_len);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)new_purpose, purpose_len);
   xpng_save_int_32(buf, X0);
   xpng_save_int_32(buf + 4, X1);
   buf[8] = (xpng_byte)type;
   buf[9] = (xpng_byte)nparams;
   xpng_write_chunk_data(xpng_ptr, buf, (xpng_size_t)10);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)units, (xpng_size_t)units_len);

   xpng_free(xpng_ptr, new_purpose);

   for (i = 0; i < nparams; i++)
   {
      xpng_write_chunk_data(xpng_ptr, (xpng_bytep)params[i],
         (xpng_size_t)params_len[i]);
   }

   xpng_free(xpng_ptr, params_len);
   xpng_write_chunk_end(xpng_ptr);
}
#endif

#if defined(PNG_WRITE_sCAL_SUPPORTED)
/* write the sCAL chunk */
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_write_sCAL(xpng_structp xpng_ptr, int unit, double width,double height)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_sCAL;
#endif
   xpng_size_t total_len;
   char wbuf[32], hbuf[32];

   xpng_debug(1, "in xpng_write_sCAL\n");

   sprintf(wbuf, "%12.12e", width);
   sprintf(hbuf, "%12.12e", height);
   total_len = 1 + xpng_strlen(wbuf)+1 + xpng_strlen(hbuf);

   xpng_debug1(3, "sCAL total length = %d\n", total_len);
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_sCAL, (xpng_uint_32)total_len);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)&unit, 1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)wbuf, strlen(wbuf)+1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)hbuf, strlen(hbuf));

   xpng_write_chunk_end(xpng_ptr);
}
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
void
xpng_write_sCAL_s(xpng_structp xpng_ptr, int unit, xpng_charp width,
   xpng_charp height)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_sCAL;
#endif
   xpng_size_t total_len;
   char wbuf[32], hbuf[32];

   xpng_debug(1, "in xpng_write_sCAL\n");

   sprintf(wbuf, "%s", width);
   sprintf(hbuf, "%s", height);
   total_len = 1 + xpng_strlen(wbuf)+1 + xpng_strlen(hbuf);

   xpng_debug1(3, "sCAL total length = %d\n", total_len);
   xpng_write_chunk_start(xpng_ptr, (xpng_bytep)xpng_sCAL, (xpng_uint_32)total_len);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)&unit, 1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)wbuf, strlen(wbuf)+1);
   xpng_write_chunk_data(xpng_ptr, (xpng_bytep)hbuf, strlen(hbuf));

   xpng_write_chunk_end(xpng_ptr);
}
#endif
#endif
#endif

#if defined(PNG_WRITE_pHYs_SUPPORTED)
/* write the pHYs chunk */
void
xpng_write_pHYs(xpng_structp xpng_ptr, xpng_uint_32 x_pixels_per_unit,
   xpng_uint_32 y_pixels_per_unit,
   int unit_type)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_pHYs;
#endif
   xpng_byte buf[9];

   xpng_debug(1, "in xpng_write_pHYs\n");
   if (unit_type >= PNG_RESOLUTION_LAST)
      xpng_warning(xpng_ptr, "Unrecognized unit type for pHYs chunk");

   xpng_save_uint_32(buf, x_pixels_per_unit);
   xpng_save_uint_32(buf + 4, y_pixels_per_unit);
   buf[8] = (xpng_byte)unit_type;

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_pHYs, buf, (xpng_size_t)9);
}
#endif

#if defined(PNG_WRITE_tIME_SUPPORTED)
/* Write the tIME chunk.  Use either xpng_convert_from_struct_tm()
 * or xpng_convert_from_time_t(), or fill in the structure yourself.
 */
void
xpng_write_tIME(xpng_structp xpng_ptr, xpng_timep mod_time)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_tIME;
#endif
   xpng_byte buf[7];

   xpng_debug(1, "in xpng_write_tIME\n");
   if (mod_time->month  > 12 || mod_time->month  < 1 ||
       mod_time->day    > 31 || mod_time->day    < 1 ||
       mod_time->hour   > 23 || mod_time->second > 60)
   {
      xpng_warning(xpng_ptr, "Invalid time specified for tIME chunk");
      return;
   }

   xpng_save_uint_16(buf, mod_time->year);
   buf[2] = mod_time->month;
   buf[3] = mod_time->day;
   buf[4] = mod_time->hour;
   buf[5] = mod_time->minute;
   buf[6] = mod_time->second;

   xpng_write_chunk(xpng_ptr, (xpng_bytep)xpng_tIME, buf, (xpng_size_t)7);
}
#endif

/* initializes the row writing capability of libpng */
void
xpng_write_start_row(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   int xpng_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   /* start of interlace block in the y direction */
   int xpng_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   /* offset to next interlace block in the y direction */
   int xpng_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   xpng_size_t buf_size;

   xpng_debug(1, "in xpng_write_start_row\n");
   buf_size = (xpng_size_t)(((xpng_ptr->width * xpng_ptr->usr_channels *
                            xpng_ptr->usr_bit_depth + 7) >> 3) + 1);

   /* set up row buffer */
   xpng_ptr->row_buf = (xpng_bytep)xpng_malloc(xpng_ptr, (xpng_uint_32)buf_size);
   xpng_ptr->row_buf[0] = PNG_FILTER_VALUE_NONE;

   /* set up filtering buffer, if using this filter */
   if (xpng_ptr->do_filter & PNG_FILTER_SUB)
   {
      xpng_ptr->sub_row = (xpng_bytep)xpng_malloc(xpng_ptr,
         (xpng_ptr->rowbytes + 1));
      xpng_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
   }

   /* We only need to keep the previous row if we are using one of these. */
   if (xpng_ptr->do_filter & (PNG_FILTER_AVG | PNG_FILTER_UP | PNG_FILTER_PAETH))
   {
     /* set up previous row buffer */
      xpng_ptr->prev_row = (xpng_bytep)xpng_malloc(xpng_ptr, (xpng_uint_32)buf_size);
      xpng_memset(xpng_ptr->prev_row, 0, buf_size);

      if (xpng_ptr->do_filter & PNG_FILTER_UP)
      {
         xpng_ptr->up_row = (xpng_bytep )xpng_malloc(xpng_ptr,
            (xpng_ptr->rowbytes + 1));
         xpng_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
      }

      if (xpng_ptr->do_filter & PNG_FILTER_AVG)
      {
         xpng_ptr->avg_row = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_ptr->rowbytes + 1));
         xpng_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
      }

      if (xpng_ptr->do_filter & PNG_FILTER_PAETH)
      {
         xpng_ptr->paeth_row = (xpng_bytep )xpng_malloc(xpng_ptr,
            (xpng_ptr->rowbytes + 1));
         xpng_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
      }
   }

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   /* if interlaced, we need to set up width and height of pass */
   if (xpng_ptr->interlaced)
   {
      if (!(xpng_ptr->transformations & PNG_INTERLACE))
      {
         xpng_ptr->num_rows = (xpng_ptr->height + xpng_pass_yinc[0] - 1 -
            xpng_pass_ystart[0]) / xpng_pass_yinc[0];
         xpng_ptr->usr_width = (xpng_ptr->width + xpng_pass_inc[0] - 1 -
            xpng_pass_start[0]) / xpng_pass_inc[0];
      }
      else
      {
         xpng_ptr->num_rows = xpng_ptr->height;
         xpng_ptr->usr_width = xpng_ptr->width;
      }
   }
   else
#endif
   {
      xpng_ptr->num_rows = xpng_ptr->height;
      xpng_ptr->usr_width = xpng_ptr->width;
   }
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
   xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
}

/* Internal use only.  Called when finished processing a row of data. */
void
xpng_write_finish_row(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   int xpng_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   /* start of interlace block in the y direction */
   int xpng_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   /* offset to next interlace block in the y direction */
   int xpng_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   int ret;

   xpng_debug(1, "in xpng_write_finish_row\n");
   /* next row */
   xpng_ptr->row_number++;

   /* see if we are done */
   if (xpng_ptr->row_number < xpng_ptr->num_rows)
      return;

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   /* if interlaced, go to next pass */
   if (xpng_ptr->interlaced)
   {
      xpng_ptr->row_number = 0;
      if (xpng_ptr->transformations & PNG_INTERLACE)
      {
         xpng_ptr->pass++;
      }
      else
      {
         /* loop until we find a non-zero width or height pass */
         do
         {
            xpng_ptr->pass++;
            if (xpng_ptr->pass >= 7)
               break;
            xpng_ptr->usr_width = (xpng_ptr->width +
               xpng_pass_inc[xpng_ptr->pass] - 1 -
               xpng_pass_start[xpng_ptr->pass]) /
               xpng_pass_inc[xpng_ptr->pass];
            xpng_ptr->num_rows = (xpng_ptr->height +
               xpng_pass_yinc[xpng_ptr->pass] - 1 -
               xpng_pass_ystart[xpng_ptr->pass]) /
               xpng_pass_yinc[xpng_ptr->pass];
            if (xpng_ptr->transformations & PNG_INTERLACE)
               break;
         } while (xpng_ptr->usr_width == 0 || xpng_ptr->num_rows == 0);

      }

      /* reset the row above the image for the next pass */
      if (xpng_ptr->pass < 7)
      {
         if (xpng_ptr->prev_row != NULL)
            xpng_memset(xpng_ptr->prev_row, 0,
               (xpng_size_t) (((xpng_uint_32)xpng_ptr->usr_channels *
               (xpng_uint_32)xpng_ptr->usr_bit_depth *
               xpng_ptr->width + 7) >> 3) + 1);
         return;
      }
   }
#endif

   /* if we get here, we've just written the last row, so we need
      to flush the compressor */
   do
   {
      /* tell the compressor we are done */
      ret = deflate(&xpng_ptr->zstream, Z_FINISH);
      /* check for an error */
      if (ret != Z_OK && ret != Z_STREAM_END)
      {
         if (xpng_ptr->zstream.msg != NULL)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg);
         else
            xpng_error(xpng_ptr, "zlib error");
      }
      /* check to see if we need more room */
      if (!(xpng_ptr->zstream.avail_out) && ret == Z_OK)
      {
         xpng_write_IDAT(xpng_ptr, xpng_ptr->zbuf, xpng_ptr->zbuf_size);
         xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
      }
   } while (ret != Z_STREAM_END);

   /* write any extra space */
   if (xpng_ptr->zstream.avail_out < xpng_ptr->zbuf_size)
   {
      xpng_write_IDAT(xpng_ptr, xpng_ptr->zbuf, xpng_ptr->zbuf_size -
         xpng_ptr->zstream.avail_out);
   }

   deflateReset(&xpng_ptr->zstream);
}

#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
/* Pick out the correct pixels for the interlace pass.
 * The basic idea here is to go through the row with a source
 * pointer and a destination pointer (sp and dp), and copy the
 * correct pixels for the pass.  As the row gets compacted,
 * sp will always be >= dp, so we should never overwrite anything.
 * See the default: case for the easiest code to understand.
 */
void
xpng_do_write_interlace(xpng_row_infop row_info, xpng_bytep row, int pass)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   int xpng_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};
#endif

   xpng_debug(1, "in xpng_do_write_interlace\n");
   /* we don't have to do anything on the last pass (6) */
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL && pass < 6)
#else
   if (pass < 6)
#endif
   {
      /* each pixel depth is handled separately */
      switch (row_info->pixel_depth)
      {
         case 1:
         {
            xpng_bytep sp;
            xpng_bytep dp;
            int shift;
            int d;
            int value;
            xpng_uint_32 i;
            xpng_uint_32 row_width = row_info->width;

            dp = row;
            d = 0;
            shift = 7;
            for (i = xpng_pass_start[pass]; i < row_width;
               i += xpng_pass_inc[pass])
            {
               sp = row + (xpng_size_t)(i >> 3);
               value = (int)(*sp >> (7 - (int)(i & 0x07))) & 0x01;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 7;
                  *dp++ = (xpng_byte)d;
                  d = 0;
               }
               else
                  shift--;

            }
            if (shift != 7)
               *dp = (xpng_byte)d;
            break;
         }
         case 2:
         {
            xpng_bytep sp;
            xpng_bytep dp;
            int shift;
            int d;
            int value;
            xpng_uint_32 i;
            xpng_uint_32 row_width = row_info->width;

            dp = row;
            shift = 6;
            d = 0;
            for (i = xpng_pass_start[pass]; i < row_width;
               i += xpng_pass_inc[pass])
            {
               sp = row + (xpng_size_t)(i >> 2);
               value = (*sp >> ((3 - (int)(i & 0x03)) << 1)) & 0x03;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 6;
                  *dp++ = (xpng_byte)d;
                  d = 0;
               }
               else
                  shift -= 2;
            }
            if (shift != 6)
                   *dp = (xpng_byte)d;
            break;
         }
         case 4:
         {
            xpng_bytep sp;
            xpng_bytep dp;
            int shift;
            int d;
            int value;
            xpng_uint_32 i;
            xpng_uint_32 row_width = row_info->width;

            dp = row;
            shift = 4;
            d = 0;
            for (i = xpng_pass_start[pass]; i < row_width;
               i += xpng_pass_inc[pass])
            {
               sp = row + (xpng_size_t)(i >> 1);
               value = (*sp >> ((1 - (int)(i & 0x01)) << 2)) & 0x0f;
               d |= (value << shift);

               if (shift == 0)
               {
                  shift = 4;
                  *dp++ = (xpng_byte)d;
                  d = 0;
               }
               else
                  shift -= 4;
            }
            if (shift != 4)
               *dp = (xpng_byte)d;
            break;
         }
         default:
         {
            xpng_bytep sp;
            xpng_bytep dp;
            xpng_uint_32 i;
            xpng_uint_32 row_width = row_info->width;
            xpng_size_t pixel_bytes;

            /* start at the beginning */
            dp = row;
            /* find out how many bytes each pixel takes up */
            pixel_bytes = (row_info->pixel_depth >> 3);
            /* loop through the row, only looking at the pixels that
               matter */
            for (i = xpng_pass_start[pass]; i < row_width;
               i += xpng_pass_inc[pass])
            {
               /* find out where the original pixel is */
               sp = row + (xpng_size_t)i * pixel_bytes;
               /* move the pixel */
               if (dp != sp)
                  xpng_memcpy(dp, sp, pixel_bytes);
               /* next pixel */
               dp += pixel_bytes;
            }
            break;
         }
      }
      /* set new row width */
      row_info->width = (row_info->width +
         xpng_pass_inc[pass] - 1 -
         xpng_pass_start[pass]) /
         xpng_pass_inc[pass];
         row_info->rowbytes = ((row_info->width *
            row_info->pixel_depth + 7) >> 3);
   }
}
#endif

/* This filters the row, chooses which filter to use, if it has not already
 * been specified by the application, and then writes the row out with the
 * chosen filter.
 */
#define PNG_MAXSUM (~((xpng_uint_32)0) >> 1)
#define PNG_HISHIFT 10
#define PNG_LOMASK ((xpng_uint_32)0xffffL)
#define PNG_HIMASK ((xpng_uint_32)(~PNG_LOMASK >> PNG_HISHIFT))
void
xpng_write_find_filter(xpng_structp xpng_ptr, xpng_row_infop row_info)
{
   xpng_bytep prev_row, best_row, row_buf;
   xpng_uint_32 mins, bpp;
   xpng_byte filter_to_do = xpng_ptr->do_filter;
   xpng_uint_32 row_bytes = row_info->rowbytes;
#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   int num_p_filters = (int)xpng_ptr->num_prev_filters;
#endif

   xpng_debug(1, "in xpng_write_find_filter\n");
   /* find out how many bytes offset each pixel is */
   bpp = (row_info->pixel_depth + 7) / 8;

   prev_row = xpng_ptr->prev_row;
   best_row = row_buf = xpng_ptr->row_buf;
   mins = PNG_MAXSUM;

   /* The prediction method we use is to find which method provides the
    * smallest value when summing the absolute values of the distances
    * from zero, using anything >= 128 as negative numbers.  This is known
    * as the "minimum sum of absolute differences" heuristic.  Other
    * heuristics are the "weighted minimum sum of absolute differences"
    * (experimental and can in theory improve compression), and the "zlib
    * predictive" method (not implemented yet), which does test compressions
    * of lines using different filter methods, and then chooses the
    * (series of) filter(s) that give minimum compressed data size (VERY
    * computationally expensive).
    *
    * GRR 980525:  consider also
    *   (1) minimum sum of absolute differences from running average (i.e.,
    *       keep running sum of non-absolute differences & count of bytes)
    *       [track dispersion, too?  restart average if dispersion too large?]
    *  (1b) minimum sum of absolute differences from sliding average, probably
    *       with window size <= deflate window (usually 32K)
    *   (2) minimum sum of squared differences from zero or running average
    *       (i.e., ~ root-mean-square approach)
    */


   /* We don't need to test the 'no filter' case if this is the only filter
    * that has been chosen, as it doesn't actually do anything to the data.
    */
   if ((filter_to_do & PNG_FILTER_NONE) &&
       filter_to_do != PNG_FILTER_NONE)
   {
      xpng_bytep rp;
      xpng_uint_32 sum = 0;
      xpng_uint_32 i;
      int v;

      for (i = 0, rp = row_buf + 1; i < row_bytes; i++, rp++)
      {
         v = *rp;
         sum += (v < 128) ? v : 256 - v;
      }

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         xpng_uint_32 sumhi, sumlo;
         int j;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK; /* Gives us some footroom */

         /* Reduce the sum if we match any of the previous rows */
         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         /* Factor in the cost of this filter (this is here for completeness,
          * but it makes no sense to have a "cost" for the NONE filter, as
          * it has the minimum possible computational cost - none).
          */
         sumlo = (sumlo * xpng_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * xpng_ptr->filter_costs[PNG_FILTER_VALUE_NONE]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif
      mins = sum;
   }

   /* sub filter */
   if (filter_to_do == PNG_FILTER_SUB)
   /* it's the only filter so no testing is needed */
   {
      xpng_bytep rp, lp, dp;
      xpng_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         *dp = *rp;
      }
      for (lp = row_buf + 1; i < row_bytes;
         i++, rp++, lp++, dp++)
      {
         *dp = (xpng_byte)(((int)*rp - (int)*lp) & 0xff);
      }
      best_row = xpng_ptr->sub_row;
   }

   else if (filter_to_do & PNG_FILTER_SUB)
   {
      xpng_bytep rp, dp, lp;
      xpng_uint_32 sum = 0, lmins = mins;
      xpng_uint_32 i;
      int v;

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      /* We temporarily increase the "minimum sum" by the factor we
       * would reduce the sum of this filter, so that we can do the
       * early exit comparison without scaling the sum each time.
       */
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               lmlo = (lmlo * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->sub_row + 1; i < bpp;
           i++, rp++, dp++)
      {
         v = *dp = *rp;

         sum += (v < 128) ? v : 256 - v;
      }
      for (lp = row_buf + 1; i < row_info->rowbytes;
         i++, rp++, lp++, dp++)
      {
         v = *dp = (xpng_byte)(((int)*rp - (int)*lp) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  /* We are already worse, don't continue. */
            break;
      }

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_SUB)
            {
               sumlo = (sumlo * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_SUB]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = xpng_ptr->sub_row;
      }
   }

   /* up filter */
   if (filter_to_do == PNG_FILTER_UP)
   {
      xpng_bytep rp, dp, pp;
      xpng_uint_32 i;

      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->up_row + 1,
           pp = prev_row + 1; i < row_bytes;
           i++, rp++, pp++, dp++)
      {
         *dp = (xpng_byte)(((int)*rp - (int)*pp) & 0xff);
      }
      best_row = xpng_ptr->up_row;
   }

   else if (filter_to_do & PNG_FILTER_UP)
   {
      xpng_bytep rp, dp, pp;
      xpng_uint_32 sum = 0, lmins = mins;
      xpng_uint_32 i;
      int v;


#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               lmlo = (lmlo * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->up_row + 1,
           pp = prev_row + 1; i < row_bytes; i++)
      {
         v = *dp++ = (xpng_byte)(((int)*rp++ - (int)*pp++) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  /* We are already worse, don't continue. */
            break;
      }

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_UP)
            {
               sumlo = (sumlo * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * xpng_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * xpng_ptr->filter_costs[PNG_FILTER_VALUE_UP]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = xpng_ptr->up_row;
      }
   }

   /* avg filter */
   if (filter_to_do == PNG_FILTER_AVG)
   {
      xpng_bytep rp, dp, pp, lp;
      xpng_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (xpng_byte)(((int)*rp++ - ((int)*pp++ / 2)) & 0xff);
      }
      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         *dp++ = (xpng_byte)(((int)*rp++ - (((int)*pp++ + (int)*lp++) / 2))
                 & 0xff);
      }
      best_row = xpng_ptr->avg_row;
   }

   else if (filter_to_do & PNG_FILTER_AVG)
   {
      xpng_bytep rp, dp, pp, lp;
      xpng_uint_32 sum = 0, lmins = mins;
      xpng_uint_32 i;
      int v;

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_AVG)
            {
               lmlo = (lmlo * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->avg_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (xpng_byte)(((int)*rp++ - ((int)*pp++ / 2)) & 0xff);

         sum += (v < 128) ? v : 256 - v;
      }
      for (lp = row_buf + 1; i < row_bytes; i++)
      {
         v = *dp++ =
          (xpng_byte)(((int)*rp++ - (((int)*pp++ + (int)*lp++) / 2)) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  /* We are already worse, don't continue. */
            break;
      }

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_NONE)
            {
               sumlo = (sumlo * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * xpng_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * xpng_ptr->filter_costs[PNG_FILTER_VALUE_AVG]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         mins = sum;
         best_row = xpng_ptr->avg_row;
      }
   }

   /* Paeth filter */
   if (filter_to_do == PNG_FILTER_PAETH)
   {
      xpng_bytep rp, dp, pp, cp, lp;
      xpng_uint_32 i;
      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->paeth_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         *dp++ = (xpng_byte)(((int)*rp++ - (int)*pp++) & 0xff);
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

         p = b - c;
         pc = a - c;

#ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

         *dp++ = (xpng_byte)(((int)*rp++ - p) & 0xff);
      }
      best_row = xpng_ptr->paeth_row;
   }

   else if (filter_to_do & PNG_FILTER_PAETH)
   {
      xpng_bytep rp, dp, pp, cp, lp;
      xpng_uint_32 sum = 0, lmins = mins;
      xpng_uint_32 i;
      int v;

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 lmhi, lmlo;
         lmlo = lmins & PNG_LOMASK;
         lmhi = (lmins >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               lmlo = (lmlo * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               lmhi = (lmhi * xpng_ptr->inv_filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         lmlo = (lmlo * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;
         lmhi = (lmhi * xpng_ptr->inv_filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;

         if (lmhi > PNG_HIMASK)
            lmins = PNG_MAXSUM;
         else
            lmins = (lmhi << PNG_HISHIFT) + lmlo;
      }
#endif

      for (i = 0, rp = row_buf + 1, dp = xpng_ptr->paeth_row + 1,
           pp = prev_row + 1; i < bpp; i++)
      {
         v = *dp++ = (xpng_byte)(((int)*rp++ - (int)*pp++) & 0xff);

         sum += (v < 128) ? v : 256 - v;
      }

      for (lp = row_buf + 1, cp = prev_row + 1; i < row_bytes; i++)
      {
         int a, b, c, pa, pb, pc, p;

         b = *pp++;
         c = *cp++;
         a = *lp++;

#ifndef PNG_SLOW_PAETH
         p = b - c;
         pc = a - c;
#ifdef PNG_USE_ABS
         pa = abs(p);
         pb = abs(pc);
         pc = abs(p + pc);
#else
         pa = p < 0 ? -p : p;
         pb = pc < 0 ? -pc : pc;
         pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif
         p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;
#else /* PNG_SLOW_PAETH */
         p = a + b - c;
         pa = abs(p - a);
         pb = abs(p - b);
         pc = abs(p - c);
         if (pa <= pb && pa <= pc)
            p = a;
         else if (pb <= pc)
            p = b;
         else
            p = c;
#endif /* PNG_SLOW_PAETH */

         v = *dp++ = (xpng_byte)(((int)*rp++ - p) & 0xff);

         sum += (v < 128) ? v : 256 - v;

         if (sum > lmins)  /* We are already worse, don't continue. */
            break;
      }

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
      if (xpng_ptr->heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
      {
         int j;
         xpng_uint_32 sumhi, sumlo;
         sumlo = sum & PNG_LOMASK;
         sumhi = (sum >> PNG_HISHIFT) & PNG_HIMASK;

         for (j = 0; j < num_p_filters; j++)
         {
            if (xpng_ptr->prev_filters[j] == PNG_FILTER_VALUE_PAETH)
            {
               sumlo = (sumlo * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
               sumhi = (sumhi * xpng_ptr->filter_weights[j]) >>
                  PNG_WEIGHT_SHIFT;
            }
         }

         sumlo = (sumlo * xpng_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;
         sumhi = (sumhi * xpng_ptr->filter_costs[PNG_FILTER_VALUE_PAETH]) >>
            PNG_COST_SHIFT;

         if (sumhi > PNG_HIMASK)
            sum = PNG_MAXSUM;
         else
            sum = (sumhi << PNG_HISHIFT) + sumlo;
      }
#endif

      if (sum < mins)
      {
         best_row = xpng_ptr->paeth_row;
      }
   }

   /* Do the actual writing of the filtered row data from the chosen filter. */

   xpng_write_filtered_row(xpng_ptr, best_row);

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   /* Save the type of filter we picked this time for future calculations */
   if (xpng_ptr->num_prev_filters > 0)
   {
      int j;
      for (j = 1; j < num_p_filters; j++)
      {
         xpng_ptr->prev_filters[j] = xpng_ptr->prev_filters[j - 1];
      }
      xpng_ptr->prev_filters[j] = best_row[0];
   }
#endif
}


/* Do the actual writing of a previously filtered row. */
void
xpng_write_filtered_row(xpng_structp xpng_ptr, xpng_bytep filtered_row)
{
   xpng_debug(1, "in xpng_write_filtered_row\n");
   xpng_debug1(2, "filter = %d\n", filtered_row[0]);
   /* set up the zlib input buffer */
   xpng_ptr->zstream.next_in = filtered_row;
   xpng_ptr->zstream.avail_in = (uInt)xpng_ptr->row_info.rowbytes + 1;
   /* repeat until we have compressed all the data */
   do
   {
      int ret; /* return of zlib */

      /* compress the data */
      ret = deflate(&xpng_ptr->zstream, Z_NO_FLUSH);
      /* check for compression errors */
      if (ret != Z_OK)
      {
         if (xpng_ptr->zstream.msg != NULL)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg);
         else
            xpng_error(xpng_ptr, "zlib error");
      }

      /* see if it is time to write another IDAT */
      if (!(xpng_ptr->zstream.avail_out))
      {
         /* write the IDAT and reset the zlib output buffer */
         xpng_write_IDAT(xpng_ptr, xpng_ptr->zbuf, xpng_ptr->zbuf_size);
         xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
      }
   /* repeat until all data has been compressed */
   } while (xpng_ptr->zstream.avail_in);

   /* swap the current and previous rows */
   if (xpng_ptr->prev_row != NULL)
   {
      xpng_bytep tptr;

      tptr = xpng_ptr->prev_row;
      xpng_ptr->prev_row = xpng_ptr->row_buf;
      xpng_ptr->row_buf = tptr;
   }

   /* finish row - updates counters and flushes zlib if last row */
   xpng_write_finish_row(xpng_ptr);

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   xpng_ptr->flush_rows++;

   if (xpng_ptr->flush_dist > 0 &&
       xpng_ptr->flush_rows >= xpng_ptr->flush_dist)
   {
      xpng_write_flush(xpng_ptr);
   }
#endif
}
