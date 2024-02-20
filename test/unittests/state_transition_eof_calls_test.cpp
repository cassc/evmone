// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../utils/bytecode.hpp"
#include "state_transition.hpp"

using namespace evmc::literals;
using namespace evmone::test;

inline constexpr auto max_uint256 =
    0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff_bytes32;

TEST_F(state_transition, eof1_delegatecall2_eof1)
{
    rev = EVMC_PRAGUE;

    constexpr auto callee = 0xca11ee_address;
    pre.insert(callee,
        {
            .storage = {{0x02_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code =
                eof_bytecode(sstore(2, 0xcc_bytes32) + mstore(0, 0x010203_bytes32) + ret(0, 32), 2),
        });
    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x02_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(delegatecall2(callee) + sstore(1, returndataload(0)) + OP_STOP, 3),
        });

    expect.gas_used = 50742;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x010203_bytes32;
    // SSTORE executed on caller.
    expect.post[callee].storage[0x02_bytes32] = 0xdd_bytes32;
    expect.post[*tx.to].storage[0x02_bytes32] = 0xcc_bytes32;
}

TEST_F(state_transition, eof1_delegatecall2_legacy)
{
    rev = EVMC_PRAGUE;

    constexpr auto callee = 0xca11ee_address;
    pre.insert(callee, {
                           .code = sstore(3, 0xcc_bytes32),
                       });
    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}},
                {0x02_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(
                sstore(1, delegatecall2(callee)) + sstore(2, returndatasize()) + OP_STOP, 3),
        });

    expect.gas_used = 26894;  // Low gas usage because DELEGATECALL2 fails lightly
    // Call - light failure.
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
    // Returndata empty.
    expect.post[*tx.to].storage[0x02_bytes32] = 0x00_bytes32;
    // SSTORE not executed.
    expect.post[callee].storage[0x03_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x03_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, delegatecall2_static)
{
    rev = EVMC_PRAGUE;
    // Checks if DELEGATECALL2 forwards the "static" flag.
    constexpr auto callee1 = 0xca11ee01_address;
    constexpr auto callee2 = 0xca11ee02_address;
    pre.insert(callee2,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });
    pre.insert(callee1,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(ret(delegatecall2(callee2)), 3),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}},
                {0x02_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(
                sstore(1, staticcall2(callee1)) + sstore(2, returndataload(0)) + OP_STOP, 3),
        });
    expect.gas_used = 974995;
    // Outer call - success.
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
    // Inner call - abort.
    expect.post[*tx.to].storage[0x02_bytes32] = 0x02_bytes32;
    // SSTORE failed.
    expect.post[callee1].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.post[callee2].storage[0x01_bytes32] = 0xdd_bytes32;
}

TEST_F(state_transition, call2_failing_with_value_balance_check)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, call2(callee).input(0x0, 0xff).value(0x1)) + OP_STOP, 4),
        });
    // Fails on balance check.
    tx.gas_limit = 21000 + 12000 + 5000;

    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
    expect.post[callee].storage[0x01_bytes32] = 0xdd_bytes32;
}

TEST_F(state_transition, call2_failing_with_value_additional_cost)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, call2(callee).input(0x0, 0xff).value(0x1)) + OP_STOP, 4),
        });
    // Fails on value transfer additional cost - maximum gas limit that triggers this
    tx.gas_limit = 37639 - 1;

    expect.gas_used = tx.gas_limit;
    expect.post[*tx.to].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.post[callee].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.status = EVMC_OUT_OF_GAS;
}

TEST_F(state_transition, call2_with_value)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .balance = 0x01,
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, call2(callee).input(0x0, 0xff).value(0x1)) + OP_STOP, 4),
        });
    expect.gas_used = 37845;

    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
    expect.post[callee].storage[0x01_bytes32] = 0xcc_bytes32;
}

TEST_F(state_transition, call2_output)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(0x0a0b_bytes32), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(
                call2(callee) + sstore(1, returndatacopy(31, 30, 1) + mload(0)) + OP_STOP, 4),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x000a_bytes32;
    expect.post[callee].exists = true;
}

TEST_F(state_transition, delegatecall2_output)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(0x0a0b_bytes32), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(
                delegatecall2(callee) + sstore(1, returndatacopy(31, 30, 1) + mload(0)) + OP_STOP,
                4),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x000a_bytes32;
    expect.post[callee].exists = true;
}


TEST_F(state_transition, staticcall2_output)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(0x0a0b_bytes32), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(
                staticcall2(callee) + sstore(1, returndatacopy(31, 30, 1) + mload(0)) + OP_STOP, 4),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x000a_bytes32;
    expect.post[callee].exists = true;
}

TEST_F(state_transition, call2_value_zero_to_nonexistent_account)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, call2(callee).input(0x0, 0xff).value(0x0)) + OP_STOP, 4),
        });
    tx.gas_limit = 30000;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, call2_then_oog)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + call2(callee) + rjump(-3), 4),
        });
    // Enough to complete CALL2, OOG in infinite loop.
    tx.gas_limit = 35000;

    expect.gas_used = tx.gas_limit;
    expect.post[*tx.to].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.post[callee].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.status = EVMC_OUT_OF_GAS;
}

TEST_F(state_transition, delegatecall2_then_oog)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + OP_STOP, 2),
        });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + delegatecall2(callee) + rjump(-3), 3),
        });
    // Enough to complete DELEGATECALL2, OOG in infinite loop.
    tx.gas_limit = 35000;

    expect.gas_used = tx.gas_limit;
    expect.post[*tx.to].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.post[callee].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.status = EVMC_OUT_OF_GAS;
}

TEST_F(state_transition, staticcall2_then_oog)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(OP_STOP),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(sstore(1, 0xcc_bytes32) + staticcall2(callee) + rjump(-3), 3),
        });
    // Enough to complete STATICCALL2, OOG in infinite loop.
    tx.gas_limit = 35000;

    expect.gas_used = tx.gas_limit;
    expect.post[*tx.to].storage[0x01_bytes32] = 0xdd_bytes32;
    expect.post[callee].exists = true;
    expect.status = EVMC_OUT_OF_GAS;
}

TEST_F(state_transition, call2_input)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(eq(calldataload(0), 0x010203)), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(mstore(0, 0x010203) + call2(callee).input(0, 32) +
                                     sstore(1, returndataload(0)) + OP_STOP,
                4),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
    expect.post[callee].exists = true;
}

TEST_F(state_transition, delegatecall2_input)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(eq(calldataload(0), 0x010203)), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(mstore(0, 0x010203) + delegatecall2(callee).input(0, 32) +
                                     sstore(1, returndataload(0)) + OP_STOP,
                3),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
    expect.post[callee].exists = true;
}

TEST_F(state_transition, staticcall2_input)
{
    rev = EVMC_PRAGUE;
    constexpr auto callee = 0xca11ee_address;

    pre.insert(callee, {
                           .code = eof_bytecode(ret(eq(calldataload(0), 0x010203)), 2),
                       });

    tx.to = To;
    pre.insert(*tx.to,
        {
            .storage = {{0x01_bytes32, {.current = 0xdd_bytes32, .original = 0xdd_bytes32}}},
            .code = eof_bytecode(mstore(0, 0x010203) + staticcall2(callee).input(0, 32) +
                                     sstore(1, returndataload(0)) + OP_STOP,
                3),
        });
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
    expect.post[callee].exists = true;
}

// // TEST_F(state_transition, call_with_value_low_gas)
// // {
// //     // Create the call destination account.
// //     host.accounts[0x0000000000000000000000000000000000000000_address] = {};
// //     for (auto call_op : {OP_CALL, OP_CALLCODE})
// //     {
// //         auto code = 4 * push(0) + push(1) + 2 * push(0) + call_op + OP_POP;
// //         execute(9721, code);
// //         EXPECT_EQ(result.status_code, EVMC_SUCCESS);
// //         EXPECT_EQ(result.gas_left, 2300 - 2);
// //     }
// // }

// // TEST_F(state_transition, call_oog_after_balance_check)
// // {
// //     // Create the call destination account.
// //     host.accounts[0x0000000000000000000000000000000000000000_address] = {};
// //     for (auto op : {OP_CALL, OP_CALLCODE})
// //     {
// //         auto code = 4 * push(0) + push(1) + 2 * push(0) + op + OP_SELFDESTRUCT;
// //         execute(12420, code);
// //         EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);
// //     }
// // }

// // TEST_F(state_transition, call_oog_after_depth_check)
// // {
// //     // Create the call recipient account.
// //     host.accounts[0x0000000000000000000000000000000000000000_address] = {};
// //     msg.depth = 1024;

// //     for (auto op : {OP_CALL, OP_CALLCODE})
// //     {
// //         const auto code = 4 * push(0) + push(1) + 2 * push(0) + op + OP_SELFDESTRUCT;
// //         execute(12420, code);
// //         EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);
// //     }

// //     rev = EVMC_TANGERINE_WHISTLE;
// //     const auto code = 7 * push(0) + OP_CALL + OP_SELFDESTRUCT;
// //     execute(721, code);
// //     EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);

// //     execute(721 + 5000 - 1, code);
// //     EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);
// // }

// // TEST_F(state_transition, call_recipient_and_code_address)
// // {
// //     constexpr auto origin = 0x9900000000000000000000000000000000000099_address;
// //     constexpr auto executor = 0xee000000000000000000000000000000000000ee_address;
// //     constexpr auto recipient = 0x4400000000000000000000000000000000000044_address;

// //     msg.sender = origin;
// //     msg.recipient = executor;

// //     for (auto op : {OP_CALL, OP_CALLCODE, OP_DELEGATECALL, OP_STATICCALL})
// //     {
// //         const auto code = 5 * push(0) + push(recipient) + push(0) + op;
// //         execute(100000, code);
// //         EXPECT_GAS_USED(EVMC_SUCCESS, 721);
// //         ASSERT_EQ(host.recorded_calls.size(), 1);
// //         const auto& call = host.recorded_calls[0];
// //         EXPECT_EQ(call.recipient, (op == OP_CALL || op == OP_STATICCALL) ? recipient :
// executor);
// //         EXPECT_EQ(call.code_address, recipient);
// //         EXPECT_EQ(call.sender, (op == OP_DELEGATECALL) ? origin : executor);
// //         host.recorded_calls.clear();
// //     }
// // }

// // TEST_F(state_transition, call_value)
// // {
// //     constexpr auto origin = 0x9900000000000000000000000000000000000099_address;
// //     constexpr auto executor = 0xee000000000000000000000000000000000000ee_address;
// //     constexpr auto recipient = 0x4400000000000000000000000000000000000044_address;

// //     constexpr auto passed_value = 3;
// //     constexpr auto origin_value = 8;

// //     msg.sender = origin;
// //     msg.recipient = executor;
// //     msg.value.bytes[31] = origin_value;
// //     host.accounts[executor].set_balance(passed_value);
// //     host.accounts[recipient] = {};  // Create the call recipient account.

// //     for (auto op : {OP_CALL, OP_CALLCODE, OP_DELEGATECALL, OP_STATICCALL})
// //     {
// //         const auto has_value_arg = (op == OP_CALL || op == OP_CALLCODE);
// //         const auto value_cost = has_value_arg ? 9000 : 0;
// //         const auto expected_value = has_value_arg           ? passed_value :
// //                                     (op == OP_DELEGATECALL) ? origin_value :
// //                                                               0;

// //         const auto code =
// //             4 * push(0) + push(has_value_arg ? passed_value : 0) + push(recipient) + push(0) +
// //             op;
// //         execute(100000, code);
// //         EXPECT_GAS_USED(EVMC_SUCCESS, 721 + value_cost);
// //         ASSERT_EQ(host.recorded_calls.size(), 1);
// //         const auto& call = host.recorded_calls[0];
// //         EXPECT_EQ(call.value.bytes[31], expected_value) << op;
// //         host.recorded_calls.clear();
// //     }
// // }

// // TEST_F(state_transition, create_oog_after)
// // {
// //     rev = EVMC_CONSTANTINOPLE;
// //     for (auto op : {OP_CREATE, OP_CREATE2})
// //     {
// //         auto code = 4 * push(0) + op + OP_SELFDESTRUCT;
// //         execute(39000, code);
// //         EXPECT_STATUS(EVMC_OUT_OF_GAS);
// //     }
// // }

// // TEST_F(state_transition, returndatasize_before_call)
// // {
// //     execute(returndatasize() + ret_top());
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 17);
// //     EXPECT_OUTPUT_INT(0);
// // }

// // TEST_F(state_transition, returndatasize)
// // {
// //     const uint8_t call_output[13]{};
// //     host.call_result.output_data = std::data(call_output);

// //     const auto code = delegatecall(0) + returndatasize() + ret_top();

// //     host.call_result.status_code = EVMC_SUCCESS;
// //     host.call_result.output_size = std::size(call_output);
// //     execute(code);
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 735);
// //     EXPECT_OUTPUT_INT(std::size(call_output));

// //     host.call_result.status_code = EVMC_FAILURE;
// //     host.call_result.output_size = 1;
// //     execute(code);
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 735);
// //     EXPECT_OUTPUT_INT(1);

// //     host.call_result.status_code = EVMC_INTERNAL_ERROR;
// //     host.call_result.output_size = 0;
// //     execute(code);
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 735);
// //     EXPECT_OUTPUT_INT(0);
// // }

// // TEST_F(state_transition, returndatacopy)
// // {
// //     const auto call_output =
// //         0x497f3c9f61479c1cfa53f0373d39d2bf4e5f73f71411da62f1d6b85c03a60735_bytes32;
// //     host.call_result.output_data = std::data(call_output.bytes);
// //     host.call_result.output_size = std::size(call_output.bytes);

// //     const auto code = delegatecall(0) + returndatacopy(0, 0, 32) + ret(0, 32);
// //     execute(code);
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 742);
// //     EXPECT_EQ(bytes_view(result.output_data, result.output_size), call_output);
// // }

// // TEST_F(state_transition, returndatacopy_empty)
// // {
// //     execute(delegatecall(0) + returndatacopy(0, 0, 0) + ret(0, 32));
// //     EXPECT_GAS_USED(EVMC_SUCCESS, 739);
// //     EXPECT_OUTPUT_INT(0);
// // }

// // TEST_F(state_transition, returndatacopy_cost)
// // {
// //     const uint8_t call_output[1]{};
// //     host.call_result.output_data = std::data(call_output);
// //     host.call_result.output_size = std::size(call_output);

// //     const auto code = staticcall(0) + returndatacopy(0, 0, 1);
// //     execute(736, code);
// //     EXPECT_EQ(result.status_code, EVMC_SUCCESS);
// //     execute(735, code);
// //     EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);
// // }

// // TEST_F(state_transition, returndatacopy_outofrange)
// // {
// //     const uint8_t call_output[2]{};
// //     host.call_result.output_data = std::data(call_output);
// //     host.call_result.output_size = std::size(call_output);

// //     execute(735, staticcall(0) + returndatacopy(0, 0, 3));
// //     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

// //     execute(735, staticcall(0) + returndatacopy(0, 1, 2));
// //     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

// //     execute(735, staticcall(0) + returndatacopy(0, 2, 1));
// //     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

// //     execute(735, staticcall(0) + returndatacopy(0, 3, 0));
// //     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

// //     execute(735, staticcall(0) + returndatacopy(0, 1, 0));
// //     EXPECT_EQ(result.status_code, EVMC_SUCCESS);

// //     execute(735, staticcall(0) + returndatacopy(0, 2, 0));
// //     EXPECT_EQ(result.status_code, EVMC_SUCCESS);
// // }

// // TEST_F(state_transition, returndatacopy_outofrange_highbits)
// // {
// //     const uint8_t call_output[2]{};
// //     host.call_result.output_data = std::data(call_output);
// //     host.call_result.output_size = std::size(call_output);

// //     // Covers an incorrect cast of RETURNDATALOAD arg to `size_t` ignoring the high bits.
// //     const auto highbits =
// //         0x1000000000000000000000000000000000000000000000000000000000000000_bytes32;
// //     execute(735, staticcall(0) + returndatacopy(0, highbits, 0));
// //     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);
// // }

// TEST_F(state_transition, returndataload)
// {
//     // Not implemented in Advanced.
//     if (is_advanced())
//         return;

//     rev = EVMC_PRAGUE;
//     const auto call_output =
//         0x497f3c9f61479c1cfa53f0373d39d2bf4e5f73f71411da62f1d6b85c03a60735_bytes32;
//     host.call_result.output_data = std::data(call_output.bytes);
//     host.call_result.output_size = std::size(call_output.bytes);
//     const auto gas = 123123;
//     host.call_result.gas_left = (gas - 3 - 3 - 3 - 100) * 63 / 64;

//     const auto code = eof_bytecode(staticcall2(0) + returndataload(0) + ret_top(), 3);

//     execute(gas, code);
//     EXPECT_GAS_USED(EVMC_SUCCESS, 131);
//     EXPECT_EQ(bytes_view(result.output_data, result.output_size), call_output);
// }

// TEST_F(state_transition, returndataload_cost)
// {
//     // Not implemented in Advanced.
//     if (is_advanced())
//         return;

//     rev = EVMC_PRAGUE;
//     const uint8_t call_output[32]{};
//     host.call_result.output_data = std::data(call_output);
//     host.call_result.output_size = std::size(call_output);
//     const auto gas = 123123;
//     host.call_result.gas_left = (gas - 3 - 3 - 3 - 100) * 63 / 64;

//     const auto code = eof_bytecode(staticcall2(0) + returndataload(0) + OP_STOP, 3);
//     execute(109, code);
//     EXPECT_EQ(result.status_code, EVMC_SUCCESS);
//     execute(108, code);
//     EXPECT_EQ(result.status_code, EVMC_OUT_OF_GAS);
// }

// TEST_F(state_transition, returndataload_outofrange)
// {
//     // Not implemented in Advanced.
//     if (is_advanced())
//         return;

//     rev = EVMC_PRAGUE;
//     {
//         const uint8_t call_output[31]{};
//         host.call_result.output_data = std::data(call_output);
//         host.call_result.output_size = std::size(call_output);

//         execute(eof_bytecode(staticcall2(0) + returndataload(0) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);
//     }
//     {
//         const uint8_t call_output[32]{};
//         host.call_result.output_data = std::data(call_output);
//         host.call_result.output_size = std::size(call_output);

//         execute(eof_bytecode(staticcall2(0) + returndataload(1) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(31) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(32) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(max_uint256) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(0) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);
//     }
//     {
//         const uint8_t call_output[34]{};
//         host.call_result.output_data = std::data(call_output);
//         host.call_result.output_size = std::size(call_output);

//         execute(eof_bytecode(staticcall2(0) + returndataload(3) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(max_uint256) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(1) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(2) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);
//     }
//     {
//         const uint8_t call_output[64]{};
//         host.call_result.output_data = std::data(call_output);
//         host.call_result.output_size = std::size(call_output);

//         execute(eof_bytecode(staticcall2(0) + returndataload(33) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(max_uint256) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);


//         execute(eof_bytecode(staticcall2(0) + returndataload(1) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(31) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);

//         execute(eof_bytecode(staticcall2(0) + returndataload(32) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);
//         execute(eof_bytecode(staticcall2(0) + returndataload(0) + OP_STOP, 3));
//         EXPECT_EQ(result.status_code, EVMC_SUCCESS);
//     }
// }

// TEST_F(state_transition, returndataload_empty)
// {
//     // Not implemented in Advanced.
//     if (is_advanced())
//         return;

//     rev = EVMC_PRAGUE;
//     execute(eof_bytecode(staticcall2(0) + returndataload(0) + OP_STOP, 3));
//     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//     execute(eof_bytecode(staticcall2(0) + returndataload(1) + OP_STOP, 3));
//     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);

//     execute(eof_bytecode(staticcall2(0) + returndataload(max_uint256) + OP_STOP, 3));
//     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);
// }

// TEST_F(state_transition, returndataload_outofrange_highbits)
// {
//     // Not implemented in Advanced.
//     if (is_advanced())
//         return;

//     rev = EVMC_PRAGUE;
//     const uint8_t call_output[34]{};
//     host.call_result.output_data = std::data(call_output);
//     host.call_result.output_size = std::size(call_output);

//     // Covers an incorrect cast of RETURNDATALOAD arg to `size_t` ignoring the high bits.
//     const auto highbits =
//         0x1000000000000000000000000000000000000000000000000000000000000000_bytes32;
//     execute(eof_bytecode(staticcall2(0) + returndataload(highbits) + OP_STOP, 3));
//     EXPECT_EQ(result.status_code, EVMC_INVALID_MEMORY_ACCESS);
// }

// // TEST_F(state_transition, call_gas_refund_propagation)
// // {
// //     rev = EVMC_LONDON;
// //     host.accounts[msg.recipient].set_balance(1);
// //     host.call_result.status_code = EVMC_SUCCESS;
// //     host.call_result.gas_refund = 1;

// //     const auto code_prolog = 7 * push(1);
// //     for (const auto op :
// //         {OP_CALL, OP_CALLCODE, OP_DELEGATECALL, OP_STATICCALL, OP_CREATE, OP_CREATE2})
// //     {
// //         execute(code_prolog + op);
// //         EXPECT_STATUS(EVMC_SUCCESS);
// //         EXPECT_EQ(result.gas_refund, 1);
// //     }
// // }

// // TEST_F(state_transition, call_gas_refund_aggregation_different_calls)
// // {
// //     rev = EVMC_LONDON;
// //     host.accounts[msg.recipient].set_balance(1);
// //     host.call_result.status_code = EVMC_SUCCESS;
// //     host.call_result.gas_refund = 1;

// //     const auto a = 0xaa_address;
// //     const auto code =
// //         call(a) + callcode(a) + delegatecall(a) + staticcall(a) + create() + create2();
// //     execute(code);
// //     EXPECT_STATUS(EVMC_SUCCESS);
// //     EXPECT_EQ(result.gas_refund, 6);
// // }

// // TEST_F(state_transition, call_gas_refund_aggregation_same_calls)
// // {
// //     rev = EVMC_LONDON;
// //     host.accounts[msg.recipient].set_balance(2);
// //     host.call_result.status_code = EVMC_SUCCESS;
// //     host.call_result.gas_refund = 1;

// //     const auto code_prolog = 14 * push(1);
// //     for (const auto op :
// //         {OP_CALL, OP_CALLCODE, OP_DELEGATECALL, OP_STATICCALL, OP_CREATE, OP_CREATE2})
// //     {
// //         execute(code_prolog + 2 * op);
// //         EXPECT_STATUS(EVMC_SUCCESS);
// //         EXPECT_EQ(result.gas_refund, 2);
// //     }
// // }
