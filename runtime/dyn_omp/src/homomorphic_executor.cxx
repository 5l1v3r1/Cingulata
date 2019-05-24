/*
    (C) Copyright 2017 CEA LIST. All Rights Reserved.
    Contributor(s): Cingulata team

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#include "homomorphic_executor.hxx"

#include <iostream>

using namespace std;
using namespace std::chrono;
using namespace cingulata;

void HomomorphicExecutor::updateMeasures(const steady_clock::time_point &start,
                                         const string &name) {
  duration<double> diff =
      duration_cast<duration<double>>(steady_clock::now() - start);

  lock_guard<mutex> lck(*execMtx.at(name));
  execTime[name] += diff.count();
  execCnt[name]++;
}

void HomomorphicExecutor::printGateInfo(
    const GateProperties &gate, const Circuit::vertex_descriptor pred1,
    const Circuit::vertex_descriptor pred2) {

  lock_guard<mutex> lck(verboseMtx);
  switch (gate.type) {
  case GateType::INPUT:
    cout << gate.id << "\t= READ('" << inpsDir + gate.id + ".ct"
         << "')";
    break;
  case GateType::XOR:
    cout << gate.id << "\t= XOR(" << circuit[pred1].id << ", "
         << circuit[pred2].id << ")";
    break;
  case GateType::AND:
    cout << gate.id << "\t= AND(" << circuit[pred1].id << ", "
         << circuit[pred2].id << ")";
    break;
  case GateType::OR:
    cout << gate.id << "\t= OR(" << circuit[pred1].id << ", "
         << circuit[pred2].id << ")";
    break;
  case GateType::NOT:
    cout << gate.id << "\t= NOT(" << circuit[pred1].id << ")";
    break;
  case GateType::CONST_0:
    cout << gate.id << "\t= 0";
    break;
  case GateType::CONST_1:
    cout << gate.id << "\t= 1";
    break;
  case GateType::BUFF:
    cout << gate.id << "\t= " << circuit[pred1].id;
    break;
  case GateType::UNDEF:
    throw runtime_error("Should never arrive here, UNDEF gate type " + gate.id);
    break;
  }

  if (gate.isOutput) {
    cout << " -> WRITE('" << outsDir + gate.id + ".ct"
         << "')";
  }
  cout << endl;
}

// void HomomorphicExecutor::Copy(ObjHandle& ct, const ObjHandle& ct_cpy) {
//   steady_clock::time_point start = steady_clock::now();

//   ct = new CipherText(*ct_cpy);

//   updateMeasures(start, "COPY");
// }

void HomomorphicExecutor::Read(ObjHandle &ct, const string &fn) {
  steady_clock::time_point start = steady_clock::now();

  ct = m_exec.read(fn);

  updateMeasures(start, "READ");
}

void HomomorphicExecutor::Write(ObjHandle &ct, const string &fn) {
  steady_clock::time_point start = steady_clock::now();

  m_exec.write(ct, fn, not stringOutput);

  updateMeasures(start, "WRITE");
}

void HomomorphicExecutor::ExecuteXOR(ObjHandle &ct_res, const ObjHandle &ct_n1,
                                     const ObjHandle &ct_n2) {
  steady_clock::time_point start = steady_clock::now();

  ct_res = m_exec.op_xor(ct_n1, ct_n2);

  updateMeasures(start, "XOR");
}

void HomomorphicExecutor::ExecuteNOT(ObjHandle &ct_res,
                                     const ObjHandle &ct_n1) {
  steady_clock::time_point start = steady_clock::now();

  ct_res = m_exec.op_not(ct_n1);

  updateMeasures(start, "NOT");
}

void HomomorphicExecutor::ExecuteAND(ObjHandle &ct_res, const ObjHandle &ct_n1,
                                     const ObjHandle &ct_n2) {
  steady_clock::time_point start = steady_clock::now();

  ct_res = m_exec.op_and(ct_n1, ct_n2);

  updateMeasures(start, "AND");
}

void HomomorphicExecutor::ExecuteOR(ObjHandle &ct_res, const ObjHandle &ct_n1,
                                    const ObjHandle &ct_n2) {
  steady_clock::time_point start = steady_clock::now();

  ct_res = m_exec.op_or(ct_n1, ct_n2);

  updateMeasures(start, "OR");
}

HomomorphicExecutor::HomomorphicExecutor(BfvBitExec &p_exec,
                                         const Circuit &circuit_p,
                                         const bool verbose_p,
                                         const bool stringOutput_p)
    : m_exec(p_exec), circuit(circuit_p), verbose(verbose_p),
      stringOutput(stringOutput_p) {
  allocatedCnt = 0;

  /* Initialize execution metrics data structures */
  const string operNames[] = {"READ", "WRITE", "XOR", "AND",
                              "OR",   "NOT",   "COPY"};
  for (const string &operName : operNames) {
    execMtx[operName] = new mutex();
    execTime[operName] = 0.0;
    execCnt[operName] = 0;
  }

  /* For each circuit gate create a corresponding ciphertext pointer */
  Circuit::vertex_iterator vi, vi_end;
  for (tie(vi, vi_end) = vertices(circuit); vi != vi_end; vi++) {
    cipherTxts[*vi] = ObjHandle();
  }
}

HomomorphicExecutor::~HomomorphicExecutor() {
  for (auto it : cipherTxts) {
    if (it.second) {
      it.second.reset();
    }
  }

  for (auto it(execMtx.begin()); it != execMtx.end(); it++) {
    delete it->second;
  }
}

void HomomorphicExecutor::DeleteGateData(const Circuit::vertex_descriptor idx) {
  assert(cipherTxts[idx]);
  cipherTxts[idx].reset();
  allocatedCnt--;
}

void HomomorphicExecutor::ExecuteGate(const Circuit::vertex_descriptor idx) {
  /* Get gate properties and predecessors */
  GateProperties gate = circuit[idx];
  Circuit::vertex_descriptor pred1, pred2;

  if (in_degree(idx, circuit) >= 1) {
    pred1 = *inv_adjacent_vertices(idx, circuit).first;
    assert(cipherTxts[pred1]);
  }
  if (in_degree(idx, circuit) >= 2) {
    pred2 = *(inv_adjacent_vertices(idx, circuit).first + 1);
    assert(cipherTxts[pred2]);
  }

  if (verbose) {
    printGateInfo(gate, pred1, pred2);
  }

  /* Execute gate operation homomorphically */
  switch (gate.type) {
  case GateType::INPUT:
    Read(cipherTxts[idx], inpsDir + gate.id + ".ct");
    break;
  case GateType::XOR:
    ExecuteXOR(cipherTxts[idx], cipherTxts[pred1], cipherTxts[pred2]);
    break;
  case GateType::AND:
    ExecuteAND(cipherTxts[idx], cipherTxts[pred1], cipherTxts[pred2]);
    break;
  case GateType::OR:
    ExecuteOR(cipherTxts[idx], cipherTxts[pred1], cipherTxts[pred2]);
    break;
  case GateType::NOT:
    ExecuteNOT(cipherTxts[idx], cipherTxts[pred1]);
    break;
  case GateType::CONST_0:
    cipherTxts[idx] = m_exec.encode(0);
    break;
  case GateType::CONST_1:
    cipherTxts[idx] = m_exec.encode(1);
    break;
  case GateType::BUFF:
    cipherTxts[idx] = cipherTxts[pred1];
    break;
  default:
    throw runtime_error("Gate type " + gate.id + " is not supported");
  }

  assert(cipherTxts[idx]);

  allocatedCnt++;
  maxAllocatedCnt = max((int)allocatedCnt, maxAllocatedCnt);

  /* If gate is output write its value */
  if (gate.isOutput) {
    Write(cipherTxts[idx], outsDir + gate.id + ".ct");
  }
}

void HomomorphicExecutor::printExecTime() {
  cout << "READ time " << execTime["READ"] << " seconds, #execs "
       << execCnt["READ"] << endl;
  cout << "COPY time " << execTime["COPY"] << " seconds, #execs "
       << execCnt["COPY"] << endl;
  cout << "XOR gates execution time " << execTime["XOR"] << " seconds, #execs "
       << execCnt["XOR"] << endl;
  cout << "NOT gates execution time " << execTime["NOT"] << " seconds, #execs "
       << execCnt["NOT"] << endl;
  cout << "AND gates execution time " << execTime["AND"] << " seconds, #execs "
       << execCnt["AND"] << endl;
  cout << "OR gates execution time " << execTime["OR"] << " seconds, #execs "
       << execCnt["OR"] << endl;
  cout << "WRITE time " << execTime["WRITE"] << " seconds, #execs "
       << execCnt["WRITE"] << endl;
  cout << "Maximal number of simultaneously allocated ciphertexts "
       << maxAllocatedCnt << endl;
}
