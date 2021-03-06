#include <gtest/gtest.h>
#include <stdexcept>

#include "qt-rappor-client/encoder.h"
#include "qt-rappor-client/qt_hash_impl.h"
#include "mock_rand_impl.h"

class EncoderTest : public ::testing::Test {
  protected:
   EncoderTest() {
      encoder_id = "metric-name";
      irr_rand = std::make_shared<rappor::MockRand>();
   }

   virtual ~EncoderTest() {
     delete deps;
     delete params;
     delete encoder;
   }

   const char* encoder_id;
   std::shared_ptr<rappor::IrrRandInterface> irr_rand;
   rappor::Deps *deps;
   rappor::Params *params;
   rappor::Encoder *encoder;
   rappor::Bits bits_out;
   std::vector<uint8_t> bits_vector;
};

// Uses HmacSha256 and 32-bit outputs.
class EncoderUint32Test : public EncoderTest {
  protected:
   EncoderUint32Test() {
     deps = new rappor::Deps(rappor::Md5, "client-secret", rappor::HmacSha256,
                             irr_rand);
     params = new rappor::Params(32,    // num_bits (k)
                                 2,     // num_hashes (h)
                                 128,   // num_cohorts (m)
                                 0.25,  // probability f for PRR
                                 0.75,  // probability p for IRR
                                 0.5);  // probability q for IRR
     encoder = new rappor::Encoder(encoder_id, *params, *deps);
   }
};

// Uses HmacDrbg and variable-size vector outputs.
class EncoderUnlimTest : public EncoderTest {
 protected:
  EncoderUnlimTest() {
    deps = new rappor::Deps(rappor::Md5, "client-secret", rappor::HmacDrbg,
                            irr_rand);
    params = new rappor::Params(64,    // num_bits (k)
                                2,     // num_hashes (h)
                                128,   // num_cohorts (m)
                                0.25,  // probability f for PRR
                                0.75,  // probability p for IRR
                                0.5);  // probability q for IRR
    encoder = new rappor::Encoder(encoder_id, *params, *deps);
  }
};


///// EncoderUint32Test
TEST_F(EncoderUint32Test, EncodeStringUint32) {
  ASSERT_TRUE(encoder->EncodeString("foo", &bits_out));
  ASSERT_EQ(2281639167, bits_out);
  ASSERT_EQ(3, encoder->cohort());
}

TEST_F(EncoderUint32Test, EncodeStringUint32Cohort) {
  encoder->set_cohort(4);  // Set pre-selected cohort.
  ASSERT_TRUE(encoder->EncodeString("foo", &bits_out));
  ASSERT_EQ(2281637247, bits_out);
  ASSERT_EQ(4, encoder->cohort());
}

TEST_F(EncoderUint32Test, EncodeBitsUint32) {
  ASSERT_TRUE(encoder->EncodeBits(0x123, &bits_out));
  ASSERT_EQ(2784956095, bits_out);
  ASSERT_EQ(3, encoder->cohort());
}

// Negative tests
// num_bits is negative.
TEST_F(EncoderUint32Test, NumBitsMustBePositiveDeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(-1,    // num_bits (k) [BAD]
                              2,     // num_hashes (h)
                              128,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "num_bits must be positive");
}

// num_hashes is negative.
TEST_F(EncoderUint32Test, NumHashesMustBePositiveDeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              -1,    // num_hashes (h) [BAD]
                              128,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "num_hashes must be positive");
}

// num_cohorts is negative.
TEST_F(EncoderUint32Test, NumCohortsMustBePositiveDeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              -1,   // num_cohorts (m)  [BAD]
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "num_cohorts must be positive");
}

// Invalid probabilities.
TEST_F(EncoderUint32Test, InvalidProbabilitiesDeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  // prob_f negative.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              -0.1,  // probability f for PRR [BAD]
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_f should be between");
  // prob_f > 1.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              1.1,  // probability f for PRR [BAD]
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_f should be between");
  // prob_p < 0.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              -0.1,  // probability p for IRR [BAD]
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_p should be between");
  // prob_p > 1.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              1.1,  // probability p for IRR [BAD]
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_p should be between");
  // prob_q < 0.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              -0.1);  // probability q for IRR [BAD]
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_q should be between");
  // prob_q > 1.
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              2,     // num_hashes (h)
                              1,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              1.1);  // probability q for IRR [BAD]
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "prob_q should be between");
}

// num_bits 64 when only 32 bits are possible.
TEST_F(EncoderUint32Test, Sha256NoMoreThan32BitsDeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(64,    // num_bits (k)
                              2,     // num_hashes (h)
                              128,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "can't be greater than 32");
}

// num_hashes too high.
TEST_F(EncoderUint32Test, NumHashesNoMoreThan16DeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(32,    // num_bits (k)
                              17,     // num_hashes (h)
                              128,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "can't be greater than 16");
}

// EncoderString with 4-byte vector and HMACSHA256 and
// EncoderString with Uint32 and HMACSHA256 should match.
TEST_F(EncoderUint32Test, StringUint32AndStringVectorMatch) {
  ASSERT_TRUE(encoder->EncodeString("foo", &bits_out));
  ASSERT_EQ(2281639167, bits_out);
  std::vector<uint8_t> expected_out(4);
  expected_out[0] = (bits_out & 0xFF000000) >> 24;
  expected_out[1] = (bits_out & 0x00FF0000) >> 16;
  expected_out[2] = (bits_out & 0x0000FF00) >> 8;
  expected_out[3] = bits_out  & 0x000000FF;

  // Reset the mock randomizer.
  delete deps;
  delete encoder;
  irr_rand = std::make_shared<rappor::MockRand>();
  deps = new rappor::Deps(rappor::Md5, "client-secret", rappor::HmacSha256,
                          irr_rand);
  encoder = new rappor::Encoder(encoder_id, *params, *deps);
  ASSERT_TRUE(encoder->EncodeString("foo", &bits_vector));
  ASSERT_EQ(expected_out, bits_vector);
}

///// EncoderUnlimTest

TEST_F(EncoderUnlimTest, EncodeStringUint64) {
  static const uint8_t ex[] = { 134, 255, 11, 255, 252, 119, 240, 223 };
  std::vector<uint8_t> expected_vector(ex, ex + sizeof(ex));

  ASSERT_TRUE(encoder->EncodeString("foo", &bits_vector));
  ASSERT_EQ(expected_vector, bits_vector);
  ASSERT_EQ(93, encoder->cohort());
}

// Negative tests.
TEST_F(EncoderUnlimTest, NumBitsNotMultipleOf8DeathTest) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  delete params;
  params = new rappor::Params(63,    // num_bits (k) [BAD]
                              17,     // num_hashes (h)
                              128,   // num_cohorts (m)
                              0.25,  // probability f for PRR
                              0.75,  // probability p for IRR
                              0.5);  // probability q for IRR
  EXPECT_DEATH(rappor::Encoder(encoder_id, *params, *deps),
               "divisible by 8");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
