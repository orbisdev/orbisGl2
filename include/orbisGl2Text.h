/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev/liborbis
 */
#ifndef _ORBISGLTEXT_H_
#define _ORBISGLTEXT_H_

#include <GLES2/gl2.h>
#include <orbisGl2.h>
#ifdef __cplusplus
extern "C" {
#endif



// Image/Texture2D data loading/unloading/saving functions
Image orbisGlLoadImage(const char *fileName);                                                             // Load image from file into CPU memory (RAM)
Image orbisGlLoadImageEx(Color *pixels, int width, int height);                                           // Load image from Color array data (RGBA - 32bit)
Image orbisGlLoadImagePro(void *data, int width, int height, int format);                                 // Load image from raw data with parameters
Image orbisGlLoadImageRaw(const char *fileName, int width, int height, int format, int headerSize);       // Load image from RAW file data
Texture2D orbisGlLoadTextureFromFile(const char *fileName);                                                       // Load texture from file into GPU memory (VRAM)
Texture2D orbisGlLoadTextureFromImage(Image image);                                                       // Load texture from image data
TextureCubemap orbisGlLoadTextureCubemapFromImage(Image image, int layoutType);                                    // Load cubemap from image, multiple image cubemap layouts supported
RenderTexture2D orbisGlLoadRenderTexture(int width, int height);                                          // Load texture for rendering (framebuffer)
void orbisGlUnloadImage(Image image);                                                                     // Unload image from CPU memory (RAM)
void orbisGlUnloadTexture(Texture2D texture);                                                             // Unload texture from GPU memory (VRAM)
void orbisGlUnloadRenderTexture(RenderTexture2D target);                                                  // Unload render texture from GPU memory (VRAM)
Color *orbisGlGetImageData(Image image);                                                                  // Get pixel data from image as a Color struct array
Vector4 *orbisGlGetImageDataNormalized(Image image);                                                      // Get pixel data from image as Vector4 array (float normalized)
Rectangle orbisGlGetImageAlphaBorder(Image image, float threshold);                                       // Get image alpha border rectangle
int orbisGlGetPixelDataSize(int width, int height, int format);                                           // Get pixel data size in bytes (image or texture)
Image orbisGlGetTextureData(Texture2D texture);                                                           // Get pixel data from GPU texture and return an Image
Image orbisGlGetScreenData(void);                                                                         // Get pixel data from screen buffer and return an Image (screenshot)
void orbisGlUpdateTextureFromPixels(Texture2D texture, const void *pixels);                                         // Update GPU texture with new data

// Image manipulation functions
Image orbisGlImageCopy(Image image);                                                                      // Create an image duplicate (useful for transformations)
Image orbisGlImageFromImage(Image image, Rectangle rec);                                                  // Create an image from another image piece
void orbisGlImageToPOT(Image *image, Color fillColor);                                                    // Convert image to POT (power-of-two)
void orbisGlImageFormat(Image *image, int newFormat);                                                     // Convert image data to desired format
void orbisGlImageAlphaMask(Image *image, Image alphaMask);                                                // Apply alpha mask to image
void orbisGlImageAlphaClear(Image *image, Color color, float threshold);                                  // Clear alpha channel to desired color
void orbisGlImageAlphaCrop(Image *image, float threshold);                                                // Crop image depending on alpha value
void orbisGlImageAlphaPremultiply(Image *image);                                                          // Premultiply alpha channel
void orbisGlImageCrop(Image *image, Rectangle crop);                                                      // Crop an image to a defined rectangle
void orbisGlImageResize(Image *image, int newWidth, int newHeight);                                       // Resize image (Bicubic scaling algorithm)
void orbisGlImageResizeNN(Image *image, int newWidth,int newHeight);                                      // Resize image (Nearest-Neighbor scaling algorithm)
void orbisGlImageResizeCanvas(Image *image, int newWidth, int newHeight, int offsetX, int offsetY, Color color);  // Resize canvas and fill with color
void orbisGlImageMipmaps(Image *image);                                                                   // Generate all mipmap levels for a provided image
void orbisGlImageDither(Image *image, int rBpp, int gBpp, int bBpp, int aBpp);                            // Dither image data to 16bpp or lower (Floyd-Steinberg dithering)
Color *orbisGlImageExtractPalette(Image image, int maxPaletteSize, int *extractCount);                    // Extract color palette from image to maximum size (memory should be freed)
Image orbisGlImageText(const char *text, int fontSize, Color color);                                      // Create an image from text (default font)
Image orbisGlImageTextEx(Font font, const char *text, float fontSize, float spacing, Color tint);         // Create an image from text (custom sprite font)
void orbisGlImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);             // Draw a source image within a destination image (tint applied to source)
void orbisGlImageDrawRectangle(Image *dst, Rectangle rec, Color color);                                   // Draw rectangle within an image
void orbisGlImageDrawRectangleLines(Image *dst, Rectangle rec, int thick, Color color);                   // Draw rectangle lines within an image
void orbisGlImageDrawText(Image *dst, Vector2 position, const char *text, int fontSize, Color color);     // Draw text (default font) within an image (destination)
void orbisGlImageDrawTextEx(Image *dst, Vector2 position, Font font, const char *text, float fontSize, float spacing, Color color); // Draw text (custom sprite font) within an image (destination)
void orbisGlImageFlipVertical(Image *image);                                                              // Flip image vertically
void orbisGlImageFlipHorizontal(Image *image);                                                            // Flip image horizontally
void orbisGlImageRotateCW(Image *image);                                                                  // Rotate image clockwise 90deg
void orbisGlImageRotateCCW(Image *image);                                                                 // Rotate image counter-clockwise 90deg
void orbisGlImageColorTint(Image *image, Color color);                                                    // Modify image color: tint
void orbisGlImageColorInvert(Image *image);                                                               // Modify image color: invert
void orbisGlImageColorGrayscale(Image *image);                                                            // Modify image color: grayscale
void orbisGlImageColorContrast(Image *image, float contrast);                                             // Modify image color: contrast (-100 to 100)
void orbisGlImageColorBrightness(Image *image, int brightness);                                           // Modify image color: brightness (-255 to 255)
void orbisGlImageColorReplace(Image *image, Color color, Color replace);                                  // Modify image color: replace color

// Image generation functions
Image orbisGlGenImageColor(int width, int height, Color color);                                           // Generate image: plain color
Image orbisGlGenImageGradientV(int width, int height, Color top, Color bottom);                           // Generate image: vertical gradient
Image orbisGlGenImageGradientH(int width, int height, Color left, Color right);                           // Generate image: horizontal gradient
Image orbisGlGenImageGradientRadial(int width, int height, float density, Color inner, Color outer);      // Generate image: radial gradient
Image orbisGlGenImageChecked(int width, int height, int checksX, int checksY, Color col1, Color col2);    // Generate image: checked
Image orbisGlGenImageWhiteNoise(int width, int height, float factor);                                     // Generate image: white noise
Image orbisGlGenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);           // Generate image: perlin noise
Image orbisGlGenImageCellular(int width, int height, int tileSize);                                       // Generate image: cellular algorithm. Bigger tileSize means bigger cells

// Texture2D configuration functions
void orbisGlGenTextureMipmaps(Texture2D *texture);                                                        // Generate GPU mipmaps for a texture
void orbisGlSetTextureFilter(Texture2D texture, int filterMode);                                          // Set texture scaling filter mode
void orbisGlSetTextureWrap(Texture2D texture, int wrapMode);                                              // Set texture wrapping mode

// Texture2D drawing functions
void orbisGlDrawTexture(Texture2D texture, int posX, int posY, Color tint);                               // Draw a Texture2D
void orbisGlDrawTextureV(Texture2D texture, Vector2 position, Color tint);                                // Draw a Texture2D with position defined as Vector2
void orbisGlDrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);  // Draw a Texture2D with extended parameters
void orbisGlDrawTextureRec(Texture2D texture, Rectangle sourceRec, Vector2 position, Color tint);         // Draw a part of a texture defined by a rectangle
void orbisGlDrawTextureQuad(Texture2D texture, Vector2 tiling, Vector2 offset, Rectangle quad, Color tint);  // Draw texture quad with tiling and offset parameters
void orbisGlDrawTexturePro(Texture2D texture, Rectangle sourceRec, Rectangle destRec, Vector2 origin, float rotation, Color tint);       // Draw a part of a texture defined by a rectangle with 'pro' parameters
void orbisGlDrawTextureNPatch(Texture2D texture, NPatchInfo nPatchInfo, Rectangle destRec, Vector2 origin, float rotation, Color tint);  // Draws a texture (or part of it) that stretches or shrinks nicely


//------------------------------------------------------------------------------------
// Font Loading and Text Drawing Functions (Module: text)
//------------------------------------------------------------------------------------

// Font loading/unloading functions
void orbisGlLoadFontDefault();
Font orbisGlGetFontDefault(void);                                                            // Get the default Font
Font orbisGlLoadFont(const char *fileName);                                                  // Load font from file into GPU memory (VRAM)
Font orbisGlLoadFontEx(const char *fileName, int fontSize, int *fontChars, int charsCount);  // Load font from file with extended parameters
Font orbisGlLoadFontFromImage(Image image, Color key, int firstChar);                        // Load font from Image (XNA style)
CharInfo *orbisGlLoadFontData(const char *fileName, int fontSize, int *fontChars, int charsCount, int type); // Load font data for further use
Image orbisGlGenImageFontAtlas(const CharInfo *chars, Rectangle **recs, int charsCount, int fontSize, int padding, int packMethod);  // Generate image font atlas using chars info
void orbisGlUnloadFont(Font font);                                                           // Unload Font from GPU memory (VRAM)

// Text drawing functions
void orbisGlDrawFPS(int posX, int posY);                                                     // Shows current FPS
void orbisGlDrawText(const char *text, int posX, int posY, int fontSize, Color color);       // Draw text (using default font)
void orbisGlDrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);                // Draw text using font and additional parameters
void orbisGlDrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);   // Draw text using font inside rectangle limits
void orbisGlDrawTextRecEx(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint,
                         int selectStart, int selectLength, Color selectTint, Color selectBackTint); // Draw text using font inside rectangle limits with support for text selection
void orbisGlDrawTextCodepoint(Font font, int codepoint, Vector2 position, float scale, Color tint);   // Draw one character (codepoint)

// Text misc. functions
int orbisGlMeasureText(const char *text, int fontSize);                                      // Measure string width for default font
Vector2 orbisGlMeasureTextEx(Font font, const char *text, float fontSize, float spacing);    // Measure string size for Font
int orbisGlGetGlyphIndex(Font font, int codepoint);                                          // Get index position for a unicode character on font

// Text strings management functions (no utf8 strings, only byte chars)
// NOTE: Some strings allocate memory internally for returned strings, just be careful!
int orbisGlTextCopy(char *dst, const char *src);                                             // Copy one string to another, returns bytes copied
bool orbisGlTextIsEqual(const char *text1, const char *text2);                               // Check if two text string are equal
unsigned int orbisGlTextLength(const char *text);                                            // Get text length, checks for '\0' ending
const char *orbisGlTextFormat(const char *text, ...);                                        // Text formatting with variables (sprintf style)
const char *orbisGlTextSubtext(const char *text, int position, int length);                  // Get a piece of a text string
char *orbisGlTextReplace(char *text, const char *replace, const char *by);                   // Replace text string (memory must be freed!)
char *orbisGlTextInsert(const char *text, const char *insert, int position);                 // Insert text in a position (memory must be freed!)
const char *orbisGlTextJoin(const char **textList, int count, const char *delimiter);        // Join text strings with delimiter
const char **orbisGlTextSplit(const char *text, char delimiter, int *count);                 // Split text into multiple strings
void orbisGlTextAppend(char *text, const char *append, int *position);                       // Append text at specific position and move cursor!
int orbisGlTextFindIndex(const char *text, const char *find);                                // Find first text occurrence within a string
const char *orbisGlTextToUpper(const char *text);                      // Get upper case version of provided string
const char *orbisGlTextToLower(const char *text);                      // Get lower case version of provided string
const char *orbisGlTextToPascal(const char *text);                     // Get Pascal case notation version of provided string
int orbisGlTextToInteger(const char *text);                            // Get integer value from text (negative values not supported)
char *orbisGlTextToUtf8(int *codepoints, int length);                  // Encode text codepoint into utf8 text (memory must be freed!)

// UTF8 text strings management functions
int *orbisGlGetCodepoints(const char *text, int *count);               // Get all codepoints in a string, codepoints count returned by parameters
int orbisGlGetCodepointsCount(const char *text);                       // Get total number of characters (codepoints) in a UTF8 encoded string
int orbisGlGetNextCodepoint(const char *text, int *bytesProcessed);    // Returns next codepoint in a UTF8 encoded string; 0x3f('?') is returned on failure
const char *orbisGlCodepointToUtf8(int codepoint, int *byteLength);    // Encode codepoint into utf8 text (char array length returned as parameter)


#ifdef __cplusplus
}
#endif

#endif