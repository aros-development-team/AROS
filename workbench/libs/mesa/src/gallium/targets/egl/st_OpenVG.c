#include <proto/openvg.h>
#include "egl.h"

PUBLIC struct st_api *
st_api_create_OpenVG(void)
{
   return (struct st_api *) GetOpenVGStateTrackerApi();
}
