/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/ipv4-xpath-routing.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// This is an example TestCase.
class XpathRoutingTestCase1 : public TestCase
{
public:
  XpathRoutingTestCase1 ();
  virtual ~XpathRoutingTestCase1 ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
XpathRoutingTestCase1::XpathRoutingTestCase1 ()
  : TestCase ("XpathRouting test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
XpathRoutingTestCase1::~XpathRoutingTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
XpathRoutingTestCase1::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class XpathRoutingTestSuite : public TestSuite
{
public:
  XpathRoutingTestSuite ();
};

XpathRoutingTestSuite::XpathRoutingTestSuite ()
  : TestSuite ("xpath-routing", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new XpathRoutingTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static XpathRoutingTestSuite xpathRoutingTestSuite;

