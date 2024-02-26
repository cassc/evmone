// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../utils/bytecode.hpp"
#include "state_transition.hpp"

using namespace evmc::literals;
using namespace evmone::test;

namespace
{
constexpr bytes32 Salt{0xff};
}

TEST_F(state_transition, create2_factory)
{
    const auto factory_code =
        calldatacopy(0, 0, calldatasize()) + create2().input(0, calldatasize());
    const auto initcode = mstore8(0, push(0xFE)) + ret(0, 1);

    tx.to = To;
    tx.data = initcode;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_code});

    const auto create_address = compute_create2_address(*tx.to, {}, initcode);
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;  // CREATE caller's nonce must be bumped
    expect.post[create_address].code = bytes{0xFE};
}

TEST_F(state_transition, create_tx_empty)
{
    // The default transaction without "to" address is a create transaction.

    expect.post[compute_create_address(Sender, pre.get(Sender).nonce)] = {
        .nonce = 1, .code = bytes{}};

    // Example of checking the expected the post state MPT root hash.
    expect.state_hash = 0x8ae438f7a4a14dbc25410dfaa12e95e7b36f311ab904b4358c3b544e06df4c50_bytes32;
}

TEST_F(state_transition, create_tx)
{
    tx.data = mstore8(0, push(0xFE)) + ret(0, 1);

    const auto create_address = compute_create_address(Sender, pre.get(Sender).nonce);
    expect.post[create_address].code = bytes{0xFE};
}

TEST_F(state_transition, create2_max_nonce)
{
    tx.to = To;
    pre.insert(*tx.to, {.nonce = ~uint64_t{0}, .code = create2()});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // Nonce is unchanged.
}

TEST_F(state_transition, create_tx_with_eof_initcode)
{
    rev = EVMC_PRAGUE;

    const bytecode init_container = eof_bytecode(ret(0, 1));

    tx.data = init_container;

    expect.tx_error = EOF_CREATION_TRANSACTION;
}

TEST_F(state_transition, create_with_eof_initcode)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const bytecode init_container = eof_bytecode(ret(0, 1));
    const auto factory_code =
        mstore(0, push(init_container)) +
        // init_container will be left-padded in memory to 32 bytes
        sstore(0, create().input(32 - init_container.size(), init_container.size())) + sstore(1, 1);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_code});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create2_with_eof_initcode)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const bytecode init_container = eof_bytecode(ret(0, 1));
    const auto factory_code =
        mstore(0, push(init_container)) +
        // init_container will be left-padded in memory to 32 bytes
        sstore(0, create2().input(32 - init_container.size(), init_container.size()).salt(0xff)) +
        sstore(1, 1);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_code});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create_tx_deploying_eof)
{
    static constexpr auto create_address = 0x3442a1dec1e72f337007125aa67221498cdd759d_address;

    rev = EVMC_PRAGUE;

    const bytecode deploy_container = eof_bytecode(bytecode(OP_INVALID));
    const auto init_code = mstore(0, push(deploy_container)) +
                           // deploy_container will be left-padded in memory to 32 bytes
                           ret(32 - deploy_container.size(), deploy_container.size());

    tx.data = init_code;

    expect.status = EVMC_CONTRACT_VALIDATION_FAILURE;
    expect.post[Sender].nonce = pre.get(Sender).nonce + 1;
    expect.post[create_address].exists = false;
}

TEST_F(state_transition, create_deploying_eof)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const bytecode deploy_container = eof_bytecode(bytecode(OP_INVALID));
    const auto init_code = mstore(0, push(deploy_container)) +
                           // deploy_container will be left-padded in memory to 32 bytes
                           ret(32 - deploy_container.size(), deploy_container.size());

    const auto factory_code = mstore(0, push(init_code)) +
                              // init_code will be left-padded in memory to 32 bytes
                              sstore(0, create().input(32 - init_code.size(), init_code.size())) +
                              sstore(1, 1);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_code});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create2_deploying_eof)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const bytecode deploy_container = eof_bytecode(bytecode(OP_INVALID));
    const auto init_code = mstore(0, push(deploy_container)) +
                           // deploy_container will be left-padded in memory to 32 bytes
                           ret(32 - deploy_container.size(), deploy_container.size());

    const auto factory_code =
        mstore(0, push(init_code)) +
        // init_code will be left-padded in memory to 32 bytes
        sstore(0, create2().input(32 - init_code.size(), init_code.size()).salt((0xff))) +
        sstore(1, 1);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_code});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create3_empty_auxdata)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID)).data(deploy_data);

    const auto init_code = returncontract(0, 0, 0);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code = create3().container(0).input(0, 0).salt(Salt) + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_auxdata_equal_to_declared)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto aux_data = "aabbccddeeff"_hex;
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data.size());
    const auto deploy_container =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data, deploy_data_size);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const bytecode init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code = calldatacopy(0, 0, OP_CALLDATASIZE) +
                              create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt) +
                              ret_top();
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    tx.data = aux_data;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    const auto expected_container = eof_bytecode(bytecode(OP_INVALID)).data(deploy_data + aux_data);

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[create_address].code = expected_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_auxdata_longer_than_declared)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto aux_data1 = "aabbccdd"_hex;
    const auto aux_data2 = "eeff"_hex;
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data1.size());
    const auto deploy_container =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data, deploy_data_size);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const bytecode init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code = calldatacopy(0, 0, OP_CALLDATASIZE) +
                              create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt) +
                              ret_top();
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    tx.data = aux_data1 + aux_data2;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    const auto expected_container =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data + aux_data1 + aux_data2);

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[create_address].code = expected_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_auxdata_shorter_than_declared)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto aux_data = "aabbccddeeff"_hex;
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data.size() + 1);
    const auto deploy_container =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data, deploy_data_size);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const auto init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    tx.data = aux_data;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_dataloadn_referring_to_auxdata)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = bytes(64, 0);
    const auto aux_data = bytes(32, 0);
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data.size());
    // DATALOADN{64} - referring to data that will be appended as aux_data
    const auto deploy_code = bytecode(OP_DATALOADN) + "0040" + ret_top();
    const auto deploy_container = eof_bytecode(deploy_code, 2).data(deploy_data, deploy_data_size);

    const auto init_code = returncontract(0, 0, 32);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code =
        sstore(0, create3().container(0).input(0, 0).salt(Salt)) + sstore(1, 1) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    const auto expected_container = eof_bytecode(deploy_code, 2).data(deploy_data + aux_data);

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x00_bytes32] = to_bytes32(create_address);
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
    expect.post[create_address].code = expected_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_with_auxdata_and_subcontainer)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto aux_data = "aabbccddeeff"_hex;
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data.size());
    const auto deploy_container = eof_bytecode(OP_INVALID)
                                      .container(eof_bytecode(OP_INVALID))
                                      .data(deploy_data, deploy_data_size);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const bytecode init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + sstore(1, 1) +
        OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    tx.data = aux_data;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    const auto expected_container = eof_bytecode(bytecode(OP_INVALID))
                                        .container(eof_bytecode(OP_INVALID))
                                        .data(deploy_data + aux_data);

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x00_bytes32] = to_bytes32(create_address);
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
    expect.post[create_address].code = expected_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_revert_empty_returndata)
{
    rev = EVMC_PRAGUE;
    const auto init_code = revert(0, 0);
    const auto init_container = eof_bytecode(init_code, 2);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) +
        sstore(1, OP_RETURNDATASIZE) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_revert_non_empty_returndata)
{
    rev = EVMC_PRAGUE;
    const auto init_code = mstore8(0, 0xaa) + revert(0, 1);
    const auto init_container = eof_bytecode(init_code, 2);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) +
        sstore(1, OP_RETURNDATASIZE) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create3_initcontainer_aborts)
{
    rev = EVMC_PRAGUE;
    const auto init_code = bytecode{Opcode{OP_INVALID}};
    const auto init_container = eof_bytecode(init_code, 0);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_initcontainer_return)
{
    rev = EVMC_PRAGUE;
    const auto init_code = bytecode{0xaa + ret_top()};
    const auto init_container = eof_bytecode(init_code, 2);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_initcontainer_stop)
{
    rev = EVMC_PRAGUE;
    const auto init_code = bytecode{Opcode{OP_STOP}};
    const auto init_container = eof_bytecode(init_code, 0);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_deploy_container_max_size)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const auto eof_header_size =
        static_cast<int>(bytecode{eof_bytecode(Opcode{OP_INVALID})}.size() - 1);
    const auto deploy_code = (0x5fff - eof_header_size) * bytecode{Opcode{OP_JUMPDEST}} + OP_STOP;
    const bytecode deploy_container = eof_bytecode(deploy_code);
    EXPECT_EQ(deploy_container.size(), 0x6000);

    // no aux data
    const auto init_code = returncontract(0, 0, 0);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x00_bytes32] = to_bytes32(create_address);
    expect.post[create_address].code = deploy_container;
}

TEST_F(state_transition, create3_deploy_container_too_large)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const auto eof_header_size =
        static_cast<int>(bytecode{eof_bytecode(Opcode{OP_INVALID})}.size() - 1);
    const auto deploy_code = (0x6000 - eof_header_size) * bytecode{Opcode{OP_JUMPDEST}} + OP_STOP;
    const bytecode deploy_container = eof_bytecode(deploy_code);
    EXPECT_EQ(deploy_container.size(), 0x6001);

    // no aux data
    const auto init_code = returncontract(0, 0, 0);
    const auto init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_appended_data_size_larger_than_64K)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const auto aux_data = bytes(std::numeric_limits<uint16_t>::max(), 0);
    const auto deploy_data = "aa"_hex;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID)).data(deploy_data);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const bytecode init_container = eof_bytecode(init_code, 3).container(deploy_container);

    static constexpr bytes32 salt2{0xfe};
    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        // with aux data, final data size = 2**16
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) +
        // no aux_data - final data size = 1
        sstore(1, create3().container(0).salt(salt2)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    tx.data = aux_data;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 2;  // 1 successful creation + 1 hard fail
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    const auto create_address = compute_create3_address(*tx.to, salt2, init_container);
    expect.post[*tx.to].storage[0x01_bytes32] = to_bytes32(create_address);
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create3_deploy_container_with_aux_data_too_large)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const auto eof_header_size =
        static_cast<int>(bytecode{eof_bytecode(Opcode{OP_INVALID})}.size() - 1);
    const auto deploy_code = (0x5fff - eof_header_size) * bytecode{Opcode{OP_JUMPDEST}} + OP_STOP;
    const bytecode deploy_container = eof_bytecode(deploy_code);
    EXPECT_EQ(deploy_container.size(), 0x6000);

    // 1 byte aux data
    const auto init_code = returncontract(0, 0, 1);
    const auto init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_nested_create3)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID)).data(deploy_data);

    const auto deploy_data_nested = "ffffff"_hex;
    const auto deploy_container_nested =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data_nested);

    const auto init_code_nested = returncontract(0, 0, 0);
    const bytecode init_container_nested =
        eof_bytecode(init_code_nested, 2).container(deploy_container_nested);

    const auto init_code = sstore(0, create3().container(1).salt(Salt)) + returncontract(0, 0, 0);
    const bytecode init_container =
        eof_bytecode(init_code, 4).container(deploy_container).container(init_container_nested);

    const auto factory_code = sstore(0, create3().container(0).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x00_bytes32] = to_bytes32(create_address);
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 2;
    const auto create_address_nested =
        compute_create3_address(create_address, Salt, init_container_nested);
    expect.post[create_address].storage[0x00_bytes32] = to_bytes32(create_address_nested);
    expect.post[create_address_nested].code = deploy_container_nested;
    expect.post[create_address_nested].nonce = 1;
}

TEST_F(state_transition, create3_nested_create3_revert)
{
    rev = EVMC_PRAGUE;

    const auto deploy_data_nested = "ffffff"_hex;
    const auto deploy_container_nested =
        eof_bytecode(bytecode(OP_INVALID)).data(deploy_data_nested);

    const auto init_code_nested = returncontract(0, 0, 0);
    const auto init_container_nested =
        eof_bytecode(init_code_nested, 2).container(deploy_container_nested);

    const auto init_code = sstore(0, create3().container(0).salt(Salt)) + revert(0, 0);
    const auto init_container = eof_bytecode(init_code, 4).container(init_container_nested);

    const auto factory_code = sstore(0, create3().container(0).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_caller_balance_too_low)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto deploy_container = eof_bytecode(bytecode{Opcode{OP_INVALID}}).data(deploy_data);

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const auto init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) +
        sstore(0, create3().container(0).input(0, OP_CALLDATASIZE).salt(Salt).value(10)) +
        sstore(1, 1) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x01_bytes32;
}

TEST_F(state_transition, create3_not_enough_gas_for_initcode_charge)
{
    rev = EVMC_PRAGUE;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID));

    const auto init_code = returncontract(0, 0, 0);
    auto init_container = eof_bytecode(init_code, 2).container(deploy_container);
    // add max size data
    const auto init_data =
        bytes(std::numeric_limits<uint16_t>::max() - bytecode(init_container).size(), 0);
    init_container.data(init_data);
    EXPECT_EQ(bytecode(init_container).size(), std::numeric_limits<uint16_t>::max());

    const auto factory_code = sstore(0, create3().container(0).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    // tx intrinsic cost + CREATE3 cost + initcode charge - not enough for pushes before CREATE3
    tx.gas_limit = 21'000 + 32'000 + (std::numeric_limits<uint16_t>::max() + 31) / 32 * 6;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.status = EVMC_OUT_OF_GAS;

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_not_enough_gas_for_mem_expansion)
{
    rev = EVMC_PRAGUE;
    auto deploy_container = eof_bytecode(bytecode(OP_INVALID));
    // max size aux data
    const auto aux_data_size = static_cast<uint16_t>(
        std::numeric_limits<uint16_t>::max() - bytecode(deploy_container).size());
    deploy_container.data({}, aux_data_size);
    EXPECT_EQ(
        bytecode(deploy_container).size() + aux_data_size, std::numeric_limits<uint16_t>::max());

    const auto init_code =
        calldatacopy(0, 0, OP_CALLDATASIZE) + returncontract(0, 0, OP_CALLDATASIZE);
    const bytecode init_container = eof_bytecode(init_code, 3).container(deploy_container);

    const auto factory_code =
        sstore(0, create3().container(0).input(0, aux_data_size).salt(Salt)) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    // tx intrinsic cost + CREATE3 cost + initcode charge + mem expansion size = not enough for
    // pushes before CREATE3
    const auto initcode_size_words = (init_container.size() + 31) / 32;
    const auto aux_data_size_words = (aux_data_size + 31) / 32;
    tx.gas_limit = 21'000 + 32'000 + static_cast<uint16_t>(6 * initcode_size_words) +
                   3 * aux_data_size_words + aux_data_size_words * aux_data_size_words / 512;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.status = EVMC_OUT_OF_GAS;

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, returncontract_not_enough_gas_for_mem_expansion)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    auto deploy_container = eof_bytecode(bytecode(OP_INVALID));
    // max size aux data
    const auto aux_data_size = static_cast<uint16_t>(
        std::numeric_limits<uint16_t>::max() - bytecode(deploy_container).size());
    deploy_container.data({}, aux_data_size);
    EXPECT_EQ(
        bytecode(deploy_container).size() + aux_data_size, std::numeric_limits<uint16_t>::max());

    const auto init_code = returncontract(0, 0, aux_data_size);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code = create3().container(0).salt(Salt) + OP_POP + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;
    // tx intrinsic cost + CREATE3 cost + initcode charge + mem expansion size = not enough for
    // pushes before RETURNDATALOAD
    const auto initcode_size_words = (init_container.size() + 31) / 32;
    const auto aux_data_size_words = (aux_data_size + 31) / 32;
    tx.gas_limit = 21'000 + 32'000 + static_cast<uint16_t>(6 * initcode_size_words) +
                   3 * aux_data_size_words + aux_data_size_words * aux_data_size_words / 512;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x00_bytes32;
}

TEST_F(state_transition, create3_clears_returndata)
{
    static constexpr auto returning_address = 0x3000_address;

    rev = EVMC_PRAGUE;
    const auto deploy_container = eof_bytecode(OP_STOP);

    const auto init_code = returncontract(0, 0, 0);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code = sstore(0, call(returning_address).gas(0xffffff)) +
                              sstore(1, returndatasize()) +
                              sstore(2, create3().container(0).salt(Salt)) +
                              sstore(3, returndatasize()) + sstore(4, 1) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 7).container(init_container);

    const auto returning_code = ret(0, 10);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});
    pre.insert(returning_address, {.nonce = 1, .code = returning_code});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    expect.post[*tx.to].storage[0x00_bytes32] = 0x01_bytes32;
    expect.post[*tx.to].storage[0x01_bytes32] = 0x0a_bytes32;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x02_bytes32] = to_bytes32(create_address);
    expect.post[*tx.to].storage[0x03_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x04_bytes32] = 0x01_bytes32;
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 1;
    expect.post[returning_address].nonce = 1;
}

TEST_F(state_transition, create3_failure_after_create3_success)
{
    rev = EVMC_PRAGUE;
    block.gas_limit = 10'000'000;
    tx.gas_limit = block.gas_limit;
    pre.get(tx.sender).balance = tx.gas_limit * tx.max_gas_price + tx.value + 1;

    const auto deploy_container = eof_bytecode(OP_STOP);

    const auto init_code = returncontract(0, 0, 0);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    const auto factory_code = sstore(0, create3().container(0).salt(Salt)) +
                              sstore(1, create3().container(0).salt(Salt)) +  // address collision
                              sstore(2, returndatasize()) + sstore(3, 1) + OP_STOP;
    const auto factory_container = eof_bytecode(factory_code, 4).container(init_container);

    tx.to = To;

    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 2;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[*tx.to].storage[0x00_bytes32] = to_bytes32(create_address);
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x02_bytes32] = 0x00_bytes32;
    expect.post[*tx.to].storage[0x03_bytes32] = 0x01_bytes32;
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create4_empty_auxdata)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = "abcdef"_hex;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID)).data(deploy_data);

    const auto init_code = returncontract(0, 0, 0);
    const bytecode init_container = eof_bytecode(init_code, 2).container(deploy_container);

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    const auto factory_code =
        create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce + 1;
    const auto create_address = compute_create3_address(*tx.to, Salt, init_container);
    expect.post[create_address].code = deploy_container;
    expect.post[create_address].nonce = 1;
}

TEST_F(state_transition, create4_invalid_initcode)
{
    rev = EVMC_PRAGUE;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID));

    const auto init_code = returncontract(0, 0, 0);
    const bytes init_container =
        eof_bytecode(init_code, 123).container(deploy_container);  // Invalid EOF

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    // TODO: extract this common code for a testing deployer contract
    const auto factory_code = create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 55752;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}

TEST_F(state_transition, create4_truncated_data_initcode)
{
    rev = EVMC_PRAGUE;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID));

    const auto init_code = returncontract(0, 0, 0);
    const bytes init_container =
        eof_bytecode(init_code, 2).data("", 1).container(deploy_container);  // Truncated data

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    // TODO: extract this common code for a testing deployer contract
    const auto factory_code = create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 55764;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}

TEST_F(state_transition, create4_invalid_deploycode)
{
    rev = EVMC_PRAGUE;
    const auto deploy_container = eof_bytecode(bytecode(OP_INVALID), 123);  // Invalid EOF

    const auto init_code = returncontract(0, 0, 0);
    const bytes init_container = eof_bytecode(init_code, 2).container(deploy_container);

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    const auto factory_code = create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 55764;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}

TEST_F(state_transition, create4_missing_initcontainer)
{
    rev = EVMC_PRAGUE;
    tx.type = Transaction::Type::initcodes;

    const auto factory_code = create4().initcode(keccak256(bytecode())).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 55236;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}

TEST_F(state_transition, create4_light_failure_stack)
{
    rev = EVMC_PRAGUE;
    tx.type = Transaction::Type::initcodes;

    const auto factory_code =
        push(0x123) + create4().value(1).initcode(0x43_bytes32).input(2, 3).salt(Salt) + push(1) +
        OP_SSTORE +  // store result from create4
        push(2) +
        OP_SSTORE +  // store the preceding push value, nothing else should remain on stack
        ret(0);
    const auto factory_container = eof_bytecode(factory_code, 6);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE4 has pushed 0x0 on stack
    expect.post[*tx.to].storage[0x02_bytes32] =
        0x0123_bytes32;  // CREATE4 fails but has cleared its args first
}

TEST_F(state_transition, create4_missing_deploycontainer)
{
    rev = EVMC_PRAGUE;
    const auto init_code = returncontract(0, 0, 0);
    const bytes init_container = eof_bytecode(init_code, 2);

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    const auto factory_code = create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 55494;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}

TEST_F(state_transition, create4_deploy_code_with_dataloadn_invalid)
{
    rev = EVMC_PRAGUE;
    const auto deploy_data = bytes(32, 0);
    // DATALOADN{64} - referring to offset out of bounds even after appending aux_data later
    const auto deploy_code = bytecode(OP_DATALOADN) + "0040" + ret_top();
    const auto aux_data = bytes(32, 0);
    const auto deploy_data_size = static_cast<uint16_t>(deploy_data.size() + aux_data.size());
    const auto deploy_container = eof_bytecode(deploy_code, 2).data(deploy_data, deploy_data_size);

    const auto init_code = returncontract(0, 0, 0);
    const bytes init_container = eof_bytecode(init_code, 2).container(deploy_container);

    tx.type = Transaction::Type::initcodes;
    tx.initcodes.push_back(init_container);

    const auto factory_code = create4().initcode(keccak256(init_container)).input(0, 0).salt(Salt) +
                              OP_DUP1 + push(1) + OP_SSTORE + ret_top();
    const auto factory_container = eof_bytecode(factory_code, 5);

    tx.to = To;
    pre.insert(*tx.to, {.nonce = 1, .code = factory_container});

    expect.gas_used = 56030;

    expect.post[tx.sender].nonce = pre.get(tx.sender).nonce + 1;
    expect.post[*tx.to].nonce = pre.get(*tx.to).nonce;  // CREATE caller's nonce must not be bumped
    expect.post[*tx.to].storage[0x01_bytes32] = 0x00_bytes32;  // CREATE must fail
}
