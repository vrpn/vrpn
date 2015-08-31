// Test code for vrpn_LamportClock

#ifdef  VRPN_USE_OLD_STREAMS
        #include <iostream.h>
#else
        #include <iostream>
        using namespace std;
#endif

#include <assert.h>

#include <vrpn_LamportClock.h>



vrpn_LamportClock * clockA;
vrpn_LamportClock * clockB;


void setUp (void) {

  clockA = new vrpn_LamportClock (2, 0);
  clockB = new vrpn_LamportClock (2, 1);

}

void tearDown (void) {

  if (clockA) delete clockA;
  if (clockB) delete clockB;

}

void test_one_getTimestampAndAdvance (void) {

  vrpn_LamportTimestamp * t1 = clockA->getTimestampAndAdvance();
  vrpn_LamportTimestamp tc (*t1);

  assert(t1);

  assert(t1->size() == 2);
  assert((*t1)[0] == 1);
  assert((*t1)[1] == 0);

  assert(!(*t1 < *t1));

  assert(!(tc < *t1));
  assert(!(*t1 < tc));

  vrpn_LamportTimestamp * t2 = clockA->getTimestampAndAdvance();

  assert(t2);

  assert((*t1)[0] == 1);
  assert((*t1)[1] == 0);
  assert(t2->size() == 2);
  assert((*t2)[0] == 2);
  assert((*t2)[1] == 0);

  assert(*t1 < *t2);
  assert(!(*t2 < *t1));

  vrpn_LamportTimestamp * t3 = clockA->getTimestampAndAdvance();
  vrpn_LamportTimestamp * t4 = clockA->getTimestampAndAdvance();
  vrpn_LamportTimestamp * t5 = clockA->getTimestampAndAdvance();

  assert(t5->size() == 2);
  assert((*t5)[0] == 5);
  assert((*t5)[1] == 0);
  
  delete t1;
  delete t2;
  delete t3;
  delete t4;
  delete t5;
}

void test_two (void) {

  vrpn_LamportTimestamp * ta1 = clockA->getTimestampAndAdvance();
  vrpn_LamportTimestamp * tb1 = clockB->getTimestampAndAdvance();
  clockA->receive(*tb1);
  vrpn_LamportTimestamp * ta2 = clockA->getTimestampAndAdvance();

  assert((*ta2)[0] == 2);
  assert((*ta2)[1] == 1);

  assert(*ta1 < *ta2);
  assert(*tb1 < *ta2);
  assert(!(*tb1 < *ta1));
  assert(!(*ta1 < *tb1));

  delete ta1;
  delete tb1;
  delete ta2;
}


int main (int, char **) {

  setUp();
  test_one_getTimestampAndAdvance();
  tearDown();

  setUp();
  test_two();
  tearDown();

  cout << "OK" << endl;
}

