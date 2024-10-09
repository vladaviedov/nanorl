extern "C" {
	#include <fff.h>

	// Internal deps
	#include "../src/fastload.c"

	#include "../src/terminfo.c"
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
FAKE_VALUE_FUNC(size_t, fread, void *, size_t, size_t, FILE *);
FAKE_VALUE_FUNC(int, fseek, FILE *, long, int);

static FILE *test_fopen(const char *name, const char *mode);

class Terminfo : public testing::Test {
protected:
	void SetUp() override {
		RESET_FAKE(fopen);
		RESET_FAKE(fread);
		RESET_FAKE(fseek);
		FFF_RESET_HISTORY();

		fopen_fake.custom_fake = &test_fopen;
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
	FILE *terminfo = try_open(db, bad_term);

	EXPECT_EQ(fopen_fake.call_count, 1u);
	EXPECT_EQ(terminfo, nullptr);
}

TEST_F(Terminfo, TerminfoTryOpenSuccess) {
	FILE *terminfo = try_open(db, term);

	EXPECT_EQ(fopen_fake.call_count, 1u);
	EXPECT_NE(terminfo, nullptr);
}

TEST_F(Terminfo, TerminfoParseCorruptHeader) {
	auto fread_lambda = [](void *ptr, size_t size, size_t nmemb, FILE *stream) -> size_t {
		EXPECT_NE(ptr, nullptr);
		EXPECT_EQ(size * nmemb, sizeof(uint16_t) * 6);

		// Corrupt header
		std::memset(ptr, 0xab, 6);
		return 6;
	};
	fread_fake.custom_fake = fread_lambda;

	bool result = parse(nullptr);
	EXPECT_EQ(result, false);
	EXPECT_EQ(fread_fake.call_count, 1u);
}

TEST_F(Terminfo, TerminfoParseSuccess16) {
	auto fread_header = [](void *ptr, size_t size, size_t nmemb, FILE *stream) -> size_t {
		EXPECT_NE(ptr, nullptr);
		EXPECT_EQ(size, sizeof(uint16_t));
		EXPECT_EQ(nmemb, 6u);

		// Header with 16bit magic
		uint16_t *header = (uint16_t *)ptr;
		header[0] = 0432;
		header[1] = 2;
		header[2] = 2;
		header[3] = 4;
		header[4] = 1;
		header[5] = 3;

		return 6;
	};
	auto fread_strings = [](void *ptr, size_t size, size_t nmemb, FILE *stream) -> size_t {
		EXPECT_NE(ptr, nullptr);
		EXPECT_EQ(size, sizeof(int16_t));
		EXPECT_EQ(nmemb, 1u);

		int16_t *header = (int16_t *)ptr;
		header[0] = 0;

		return 1;
	};
	auto fread_table = [](void *ptr, size_t size, size_t nmemb, FILE *stream) -> size_t {
		EXPECT_NE(ptr, nullptr);
		EXPECT_EQ(size, sizeof(char));
		EXPECT_EQ(nmemb, 3u);

		// Corrupt header
		std::memcpy(ptr, "ab", 3);

		return 3;
	};

	size_t (*fread_lambdas[])(void *, size_t, size_t, FILE *) = {
		fread_header, fread_strings, fread_table
	};
	SET_CUSTOM_FAKE_SEQ(fread, fread_lambdas, 3);

	bool result = parse(nullptr);
	EXPECT_EQ(result, true);
	EXPECT_EQ(fread_fake.call_count, 3u);
}
