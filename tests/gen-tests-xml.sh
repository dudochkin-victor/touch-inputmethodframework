#!/bin/bash

UT_TESTCASES=""
FT_TESTCASES=""

for TEST in `ls -d ?t_*`; do
	if [ -x $TEST/$TEST ]; then

TESTCASE_TEMPLATE="<case name=\"$TEST\" description=\"$TEST\" requirement=\"\" timeout=\"60\">
        <step expected_result=\"0\">LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/meego-im-framework-tests/plugins:/usr/lib/qt4/plugins/inputmethods /usr/lib/meego-im-framework-tests/$TEST/$TEST</step>
      </case>
      "

	    UT_TESTCASES="${UT_TESTCASES}${TESTCASE_TEMPLATE}"
	fi
done

TESTSUITE_TEMPLATE="<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>
<testdefinition version=\"0.1\">
  <suite name=\"meego-im-framework-tests\">
    <set name=\"unit_tests\" description=\"Unit Tests\">

      <case name=\"sleep_workaround\" description=\"ensure X server is running. remove when proper fix is in place\" requirement=\"\" timeout=\"60\">
        <step expected_result=\"0\">sleep 30</step>
      </case>

      $UT_TESTCASES

      <environments>
        <scratchbox>false</scratchbox>
        <hardware>true</hardware>
      </environments>

    </set>
  </suite>
</testdefinition>"

echo "$TESTSUITE_TEMPLATE"

