// I had to include these stubs to make the code compile under Linux.
// They will disable the hid_send_feature_report() and hid_get_feature_report()
// functions, unfortunately, but it did not link otherwise.
#include <stdio.h>
static int HIDIOCSFEATURE(size_t length) {
  fprintf(stderr,"HIDIOCSFEATURE not found during link, so hacked into vrpn_Local_HIDAPI.C.  This implementation does nothing.\n");
  return 0;
}
static int HIDIOCGFEATURE(size_t length) {
  fprintf(stderr,"HIDIOCGFEATURE not found during link, so hacked into vrpn_Local_HIDAPI.C.  This implementation does nothing.\n");
  return 0;
}
#include "submodules/hidapi/linux/hid.c"
