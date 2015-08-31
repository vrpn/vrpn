#include <stdio.h>                      // for NULL, printf, sprintf
#include <vrpn_Connection.h>            // for vrpn_Connection, etc
#include <vrpn_Shared.h>                // for vrpn_gettimeofday, timeval

#include "rpc_Test.h"                   // for rpc_Test, NAME_LENGTH
#include "rpc_Test_Remote.h"            // for rpc_Test_Remote
#include "vrpn_Types.h"                 // for vrpn_int32, vrpn_float32

int main (int, char **) {

  vrpn_Connection * connection;

  connection = vrpn_create_server_connection(4999);

  int myID = connection->register_sender("rpc_Test");

  rpc_Test * enc_out = new rpc_Test(connection);

  connection->mainloop();

  struct timeval now;
  char * buf = NULL;
  vrpn_int32 len = 0;
  vrpn_gettimeofday(&now, NULL);
  printf("Expect Empty\n");
  connection->pack_message(len, now, enc_out->d_Empty_type, myID,
                           (char *) buf, vrpn_CONNECTION_RELIABLE);
  connection->mainloop();

  buf = enc_out->encode_Simple(&len, 2, 1.85f, 3.23f);
  vrpn_gettimeofday(&now, NULL);
  printf("Expect Simple, 2, 1.85, 3.23\n");
  connection->pack_message(len, now, enc_out->d_Simple_type, myID,
                           (char *) buf, vrpn_CONNECTION_RELIABLE);
  delete [] buf; buf = NULL;
  connection->mainloop();

  const vrpn_int32 cnt = 3;
  char nm[NAME_LENGTH] = "Jones";
  char nm2[cnt]= "nM";
  char doublenm[4][NAME_LENGTH] = {"one", "two", "three", "four"};
  int i;

//    char *p_to_char[4]= {"one", "two", "three", "four"};
//    char (*arr_of_p)[4] ;
//    printf ("size %d %d\n", sizeof(p_to_char), sizeof(arr_of_p));

  char **triplenm[4]; //char **(*p_t)[4] = &triplenm;
  for (i =0; i<4; i++) {
      triplenm[i] = new char *[cnt];
      for (int j=0; j< cnt; j++) {
          triplenm[i][j] = new char[cnt];
          sprintf (triplenm[i][j], "%c%c", (char)((int)'a' + i),
                   (char)((int)'j' + j)); 
      }
  }
//    char triplenm[4][cnt][cnt] =   { {"aa", "ab", "ac" } ,
//                                     {"ba", "bb", "bc" } ,
//                                     {"ca", "cb", "cc" } ,
//                                     {"da", "db", "dc" } };
  buf = enc_out->encode_CharArray(&len, cnt, nm, nm2, 
                                  doublenm, triplenm);
  vrpn_gettimeofday(&now, NULL);
  printf("Expect CharArray ...\n");
  connection->pack_message(len, now, enc_out->d_CharArray_type, myID,
                           (char *) buf, vrpn_CONNECTION_RELIABLE);
  delete [] buf; buf = NULL;
  connection->mainloop();

  //  vrpn_int32 cnt;
  vrpn_int32 shortstuff[] = {1,2,3};
  vrpn_int32 constdouble[6][4];
  vrpn_int32 *triple[4][NAME_LENGTH];
  for ( i =0; i<6; i++) {
      for (int j=0; j< 4; j++) {
          constdouble[i][j] = i+j;
      }
  }
  for ( i =0; i<4; i++) {
      for (int j=0; j< NAME_LENGTH; j++) {
          triple[i][j] = new vrpn_int32[cnt];
          for (int k=0; k< cnt; k++) {
              triple[i][j][k] = i+j+k;
          }
      }
  }

  buf = enc_out->encode_IntArray(&len, cnt, &(shortstuff[0]),
                                  constdouble, triple);
  vrpn_gettimeofday(&now, NULL);
  printf("Expect IntArray ...\n");
  connection->pack_message(len, now, enc_out->d_IntArray_type, myID,
                           (char *) buf, vrpn_CONNECTION_RELIABLE);
  delete [] buf; buf = NULL;
  connection->mainloop();
  

  vrpn_int32 count = 3;
  char name[64] = "Z Piezo";
  char units[64]= "nm";
  vrpn_float32 offset = 1.3f;
  vrpn_float32 scale = 0.025f;

  char * mptr; int mlen;
  buf = enc_out->encode_ReportScanDatasets_header ( &len, &mptr, &mlen, count );
  for (int lv_1 = 0; lv_1 < count; lv_1++) {
   enc_out->encode_ReportScanDatasets_body ( &len, buf, &mptr, &mlen, 
                                             name, units, offset, scale );
   offset *= 2;
   scale *=3;
  }

  vrpn_gettimeofday(&now, NULL);
  printf("Expect ReportScanDatasets ...\n");
  connection->pack_message(len, now, enc_out->d_ReportScanDatasets_type, myID,
                           (char *) buf, vrpn_CONNECTION_RELIABLE);
  delete [] buf; buf = NULL;
  connection->mainloop();

  return 0;
}

