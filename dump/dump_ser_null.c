/**
 *  simple serialiser: very simple example of serialising PNFS data.
 */
#include <stdio.h>
#include <string.h>

#include "dump_serialiser.h"

#include "dump_ser_null.h"

static dump_serialiser_t info = {
  "produces no output",
  FLAGS_REQ_DEFAULT,
  NULL, NULL,   /* ignore start and end of serialisation */
  NULL, NULL,   /* ignore begin & end dump */
  NULL, NULL,   /* ignore beginning and end of directories */
  NULL, NULL, NULL, NULL, /* ignore all tags */
  NULL, NULL,   /* ignore all files */
  NULL,         /* ignore all levels */
  NULL,         /* ignore all symbolic links */
};


dump_serialiser_t *null_get_serialiser()
{
  return &info;
}
