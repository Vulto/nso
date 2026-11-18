/* Star Fox's desktop launcher supplies this implementation. The NSO module
 * omits that launcher but keeps the renderer's optional PNG capture symbols
 * resolvable for RTLD_NOW. */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
