// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MEDIAPIPE_CALCULATORS_CORE_CONCATENATE_NORMALIZED_LIST_CALCULATOR_H_  // NOLINT
#define MEDIAPIPE_CALCULATORS_CORE_CONCATENATE_NORMALIZED_LIST_CALCULATOR_H_  // NOLINT

#include "mediapipe/framework/api2/node.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/canonical_errors.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"

#include <iostream>
#include <winsock2.h>
#include "ws2tcpip.h"
#pragma comment(lib, "ws2_32.lib")

WSADATA WSAData;
SOCKET sckt;
sockaddr_in dst;

namespace mediapipe {
namespace api2 {

std::string tags[] = {"pose", "leftHand", "rightHand", "face"};

void setupUDP() {
  WSAStartup(MAKEWORD(2, 0), &WSAData);

  sckt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr.s_addr) <= 0)
  {
      std::cout << "impossible de déterminer l'adresse.\n";
      return;
  }
  dst.sin_family = AF_INET;
  dst.sin_port = htons(8080);
}

// Concatenates several NormalizedLandmarkList protos following stream index
// order. This class assumes that every input stream contains a
// NormalizedLandmarkList proto object.
class UDPCalculator : public Node {
 public:
  static constexpr Input<NormalizedLandmarkList>::Multiple kIn{""};
  static constexpr Output<NormalizedLandmarkList> kOut{""};

  MEDIAPIPE_NODE_CONTRACT(kIn, kOut);

  static absl::Status UpdateContract(CalculatorContract* cc) {
    RET_CHECK_GE(kIn(cc).Count(), 1);
    setupUDP();
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) override {
    NormalizedLandmarkListTaggedVector vector;

    int i = 0;
    for (const auto& input : kIn(cc)) {
      if (!input.IsEmpty()){
        NormalizedLandmarkListTagged taggedList; 
        taggedList.set_tag(tags[i]);        
        NormalizedLandmarkList* tmpList = new NormalizedLandmarkList();
        const NormalizedLandmarkList& list = *input;
        tmpList->CopyFrom(list);
        taggedList.set_allocated_landmarklist(tmpList);

        vector.add_landmarklisttagged()->CopyFrom(taggedList);
      }
      i++;
    }
    if(vector.landmarklisttagged_size() > 0){
      std::string msg_buffer;
      vector.SerializeToString(&msg_buffer);
      sendto(sckt, msg_buffer.c_str(), msg_buffer.length(), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    }
    return absl::OkStatus();
  }
};
MEDIAPIPE_REGISTER_NODE(UDPCalculator);

}  // namespace api2
}  // namespace mediapipe

// NOLINTNEXTLINE
#endif  // MEDIAPIPE_CALCULATORS_CORE_UDP_CALCULATOR_H_