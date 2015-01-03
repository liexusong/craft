#include "CuTest.h"

#include "raft_log.h"
#include "raft_wire.h"
#define NODE_COUNT 5
#include "test_helpers.h"

/*******************************************************************************
 *******************************************************************************
 ************************* AppendEntries Wire Format ***************************
 *******************************************************************************
 ******************************************************************************/

static uint8_t expected_append_entries_message_no_logs[] = {
  0, 0, 1, MSG_TYPE_APPEND_ENTRIES,
  0, 0, 0, 32,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0x11, 0x22, 0x33, 0x44,
  0x00, 0x11, 0x00, 0x33,
  0, 0, 0, 0,
  0x99, 0x00, 0x22, 0x11,
};

void Test_raft_write_append_entries_envelope_With_no_log_entries(CuTest* tc) {
  raft_append_entries_args_t args = {
    .term = 0x55443322,
    .leader_id = 0x99887766,
    .prev_log_index = 0x11223344,
    .prev_log_term = 0x00110033,
    .p_log_entries = NULL,
    .leader_commit = 0x99002211,
  };

  raft_envelope_t env = { 0 };
  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_write_append_entries_envelope(&env, 2, &args));;

  CuAssertIntEquals(tc, 2, env.recipient_id);
  CuAssertIntEquals(tc, 32, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_append_entries_message_no_logs);
  CuAssertIntEquals(tc, arr_size, env.message_size);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc, expected_append_entries_message_no_logs[i], env.p_message[i]);
  }

  raft_dealloc_envelope(&env);
  CuAssertPtrEquals(tc, NULL, env.p_message);
}

static uint8_t expected_append_entries_message_one_log[] = {
  0, 0, 1, MSG_TYPE_APPEND_ENTRIES,
  0, 0, 0, (32 + 12 + 0) /* Fixed + log metadata + log data */,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0x11, 0x22, 0x33, 0x44,
  0x00, 0x11, 0x00, 0x33,
  0, 0, 0, 1,
  0x99, 0x00, 0x22, 0x11,
  0x80, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};

void Test_raft_write_append_entries_envelope_With_one_log_entry(CuTest* tc) {
  raft_append_entries_args_t args = {
    .term = 0x55443322,
    .leader_id = 0x99887766,
    .prev_log_index = 0x11223344,
    .prev_log_term = 0x00110033,
    .p_log_entries = raft_log_alloc(),
    .leader_commit = 0x99002211,
  };

  raft_envelope_t env = { 0 };
  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_write_append_entries_envelope(&env, 2, &args));;

  CuAssertIntEquals(tc, 2, env.recipient_id);
  CuAssertIntEquals(tc, (32 + 12), env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_append_entries_message_one_log);
  CuAssertIntEquals(tc, arr_size, env.message_size);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc, expected_append_entries_message_one_log[i], env.p_message[i]);
  }

  raft_dealloc_envelope(&env);
  CuAssertPtrEquals(tc, NULL, env.p_message);

  raft_log_free(args.p_log_entries);
}

static uint8_t expected_append_entries_message_two_logs[] = {
  0, 0, 1, MSG_TYPE_APPEND_ENTRIES,
  0, 0, 0, (32 + 12*2 + 7) /* Fixed + log metadata + log data */,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0x11, 0x22, 0x33, 0x44,
  0x00, 0x11, 0x00, 0x33,
  0, 0, 0, 2,
  0x99, 0x00, 0x22, 0x11,
  0x80, 0, 0, 0,          /* Entry 0 Metadata */
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 7,             /* Entry 1 Metadata */
  0xff, 0xaa, 0xcc, 0xdd,
  0, 0, 0, 1,
  0x11, 0x22, 0x33, 0x44, /* Entry 1 Data */
  0x55, 0x66, 0x77,
};

void Test_raft_write_append_entries_envelope_With_two_log_entries(CuTest* tc) {
  raft_append_entries_args_t args = {
    .term = 0x55443322,
    .leader_id = 0x99887766,
    .prev_log_index = 0x11223344,
    .prev_log_term = 0x00110033,
    .p_log_entries = raft_log_alloc(),
    .leader_commit = 0x99002211,
  };

  uint8_t* p_data = malloc(7);
  memcpy(p_data, &expected_append_entries_message_two_logs[56], 7);
  raft_log_append_user(args.p_log_entries, 0xffaaccdd, 1, p_data, 7);

  raft_envelope_t env = { 0 };
  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_write_append_entries_envelope(&env, 2, &args));;

  CuAssertIntEquals(tc, 2, env.recipient_id);
  CuAssertIntEquals(tc, 63, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_append_entries_message_two_logs);
  CuAssertIntEquals(tc, arr_size, env.message_size);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc, expected_append_entries_message_two_logs[i], env.p_message[i]);
  }

  raft_dealloc_envelope(&env);
  CuAssertPtrEquals(tc, NULL, env.p_message);

  raft_log_free(args.p_log_entries);
}

/*******************************************************************************
 *******************************************************************************
 *************************** RequestVote Wire Format ***************************
 *******************************************************************************
 ******************************************************************************/

static uint8_t expected_request_vote_message[] = {
  0, 0, 1, MSG_TYPE_REQUEST_VOTE,
  0, 0, 0, 24,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0x11, 0x22, 0x33, 0x44,
  0x00, 0x11, 0x00, 0x33,
};

void Test_raft_write_request_vote_envelope(CuTest* tc) {
  raft_request_vote_args_t args = {
    .term = 0x55443322,
    .candidate_id = 0x99887766,
    .last_log_index = 0x11223344,
    .last_log_term = 0x00110033
  };

  raft_envelope_t env = { 0 };
  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_write_request_vote_envelope(&env, 1, &args));

  CuAssertIntEquals(tc, 1, env.recipient_id);
  CuAssertIntEquals(tc, 24, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_request_vote_message);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc, expected_request_vote_message[i], env.p_message[i]);
  }

  raft_dealloc_envelope(&env);
  CuAssertPtrEquals(tc, NULL, env.p_message);
}

void Test_raft_read_request_vote_message(CuTest* tc) {
  raft_request_vote_args_t args = { 0 };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_read_request_vote_args(
                        &args,
                        expected_request_vote_message,
                        sizeof(expected_request_vote_message)));

  CuAssertIntEquals(tc, 0x55443322, args.term);
  CuAssertIntEquals(tc, 0x99887766, args.candidate_id);
  CuAssertIntEquals(tc, 0x11223344, args.last_log_index);
  CuAssertIntEquals(tc, 0x00110033, args.last_log_term);
}

/*******************************************************************************
 *******************************************************************************
 ********************** RequestVoteResponse Wire Format ************************
 *******************************************************************************
 ******************************************************************************/

static uint8_t expected_request_vote_response_message[] = {
  0, 0, 1, MSG_TYPE_REQUEST_VOTE_RESPONSE,
  0, 0, 0, 20,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0, 0, 0, 1,
};

void Test_raft_write_request_vote_response_envelope(CuTest* tc) {
  raft_request_vote_response_args_t args = {
    .follower_id = 0x55443322,
    .term = 0x99887766,
    .vote_granted = RAFT_TRUE
  };

  raft_envelope_t env = { 0 };
  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_write_request_vote_response_envelope(&env, 1, &args));

  CuAssertIntEquals(tc, 1, env.recipient_id);
  CuAssertIntEquals(tc, 20, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_request_vote_response_message);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc,
                      expected_request_vote_response_message[i],
                      env.p_message[i]);
  }

  raft_dealloc_envelope(&env);
  CuAssertPtrEquals(tc, NULL, env.p_message);
}

void Test_raft_read_request_vote_response_message(CuTest* tc) {
  raft_request_vote_response_args_t args = { 0 };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_read_request_vote_response_args(
                        &args,
                        expected_request_vote_response_message,
                        sizeof(expected_request_vote_response_message)));

  CuAssertIntEquals(tc, 0x55443322, args.follower_id);
  CuAssertIntEquals(tc, 0x99887766, args.term);
  CuAssertIntEquals(tc, 0x1, args.vote_granted);
}
