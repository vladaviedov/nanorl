extern "C" {
	#include "../src/terminfo.c"
	#include <fff.h>
}

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
DEFINE_FFF_GLOBALS;

static const char *db = "testdb";
static const char *term = "xterm";
static const char *bad_term = "yterm";

FAKE_VALUE_FUNC(FILE *, fopen, const char *, const char *);

class Terminfo : public testing::Test {
protected:
	void SetUp() override {
		RESET_FAKE(fopen);
		FFF_RESET_HISTORY();
	}
};

static FILE *test_fopen(const char *name, const char *mode) {
	std::ostringstream builder;
	builder << db << "/" << term[0] << "/" << term;
	std::string correct_name = builder.str();

	// Return some non-null value
	if (name == correct_name && mode == std::string("r")) {
		return (FILE *)1;
	}

	// Else, simulate an error
	return nullptr;
}

TEST_F(Terminfo, TerminfoTryOpenFail) {
	fopen_fake.custom_fake = &test_fopen;
	FILE *terminfo = try_open(db, bad_term);

	ASSERT_EQ(fopen_fake.call_count, 1u);
	ASSERT_EQ(terminfo, nullptr);
}

TEST_F(Terminfo, TerminfoTryOpenSuccess) {
	fopen_fake.custom_fake = &test_fopen;
	FILE *terminfo = try_open(db, term);

	ASSERT_EQ(fopen_fake.call_count, 1u);
	ASSERT_NE(terminfo, nullptr);
}
