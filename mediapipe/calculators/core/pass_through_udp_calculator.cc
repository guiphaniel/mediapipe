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

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/canonical_errors.h"

#include <iostream>
#include <winsock2.h>
#include "ws2tcpip.h"
#pragma comment(lib, "ws2_32.lib")

WSADATA WSAData;
SOCKET sckt;
sockaddr_in dst;

namespace mediapipe {

/*constexpr char kPoseTag[] = "MY_POSE_LANDMARKS";
constexpr char kLeftHandTag[] = "MY_LEFT_HAND_LANDMARKS";
constexpr char kRightHandTag[] = "MY_RIGHT_HAND_LANDMARKS";
constexpr char kFaceTag[] = "MY_FACE_LANDMARKS";*/

std::string tags[] = {"MY_POSE_LANDMARKS", "MY_LEFT_HAND_LANDMARKS", "MY_RIGHT_HAND_LANDMARKS", "MY_FACE_LANDMARKS"};

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

// A Calculator that simply passes its input Packets and header through,
// unchanged.  The inputs may be specified by tag or index.  The outputs
// must match the inputs exactly.  Any number of input side packets may
// also be specified.  If output side packets are specified, they must
// match the input side packets exactly and the Calculator passes its
// input side packets through, unchanged.  Otherwise, the input side
// packets will be ignored (allowing PassThroughCalculator to be used to
// test internal behavior).  Any options may be specified and will be
// ignored.
class PassThroughUDPCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc) {
    if (!cc->Inputs().TagMap()->SameAs(*cc->Outputs().TagMap())) {
      return absl::InvalidArgumentError(
          "Input and output streams to PassThroughCalculator must use "
          "matching tags and indexes.");
    }
    for (CollectionItemId id = cc->Inputs().BeginId();
         id < cc->Inputs().EndId(); ++id) {
      cc->Inputs().Get(id).SetAny();
      cc->Outputs().Get(id).SetSameAs(&cc->Inputs().Get(id));
    }
    for (CollectionItemId id = cc->InputSidePackets().BeginId();
         id < cc->InputSidePackets().EndId(); ++id) {
      cc->InputSidePackets().Get(id).SetAny();
    }
    if (cc->OutputSidePackets().NumEntries() != 0) {
      if (!cc->InputSidePackets().TagMap()->SameAs(
              *cc->OutputSidePackets().TagMap())) {
        return absl::InvalidArgumentError(
            "Input and output side packets to PassThroughCalculator must use "
            "matching tags and indexes.");
      }
      for (CollectionItemId id = cc->InputSidePackets().BeginId();
           id < cc->InputSidePackets().EndId(); ++id) {
        cc->OutputSidePackets().Get(id).SetSameAs(
            &cc->InputSidePackets().Get(id));
      }
    }

    setupUDP();

    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) final {
    for (CollectionItemId id = cc->Inputs().BeginId();
         id < cc->Inputs().EndId(); ++id) {
      if (!cc->Inputs().Get(id).Header().IsEmpty()) {
        cc->Outputs().Get(id).SetHeader(cc->Inputs().Get(id).Header());
      }
    }
    if (cc->OutputSidePackets().NumEntries() != 0) {
      for (CollectionItemId id = cc->InputSidePackets().BeginId();
           id < cc->InputSidePackets().EndId(); ++id) {
        cc->OutputSidePackets().Get(id).Set(cc->InputSidePackets().Get(id));
      }
    }
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) final {
    cc->GetCounter("PassThrough")->Increment();
    if (cc->Inputs().NumEntries() == 0) {
      return tool::StatusStop();
    }

    NormalizedLandmarkListTaggedVector vector; //create vector
    int i = 0;
    for (CollectionItemId id = cc->Inputs().BeginId();
         id < cc->Inputs().EndId(); ++id) {
      if (cc->Inputs().Get(id).IsEmpty()) {   
        std::cout << "Empty !" << std::endl;    
        NormalizedLandmarkListTagged taggedList; 
        taggedList.set_tag(cc->Inputs().Get(id).Name());        
        /*NormalizedLandmarkList* tmpList = new NormalizedLandmarkList();
        tmpList->InitAsDefaultInstance();
        taggedList.set_allocated_landmarklist(tmpList);*/
        vector.add_landmarklisttagged()->CopyFrom(taggedList);  
      } else {
        std::cout << "Fulfilled !" << std::endl;   
        NormalizedLandmarkListTagged taggedList; 
        taggedList.set_tag(cc->Inputs().Get(id).Name());        
        NormalizedLandmarkList* tmpList = new NormalizedLandmarkList();
        const NormalizedLandmarkList& list = cc->Inputs().Get(id).Get<NormalizedLandmarkList>();
        tmpList->CopyFrom(list);
        taggedList.set_allocated_landmarklist(tmpList);
        vector.add_landmarklisttagged()->CopyFrom(taggedList);       

        VLOG(3) << "Passing " << cc->Inputs().Get(id).Name() << " to "
                << cc->Outputs().Get(id).Name() << " at "
                << cc->InputTimestamp().DebugString();
        cc->Outputs().Get(id).AddPacket(cc->Inputs().Get(id).Value());
      }
      //std::cout << "n" << i << " : " << cc->Inputs().Get(id).Name() << std::endl;
      i++;
    }

    std::string msg_buffer;
    vector.SerializeToString(&msg_buffer);
    std::cout << msg_buffer << std::endl;
    Sleep(3000);
    sendto(sckt, msg_buffer.c_str(), msg_buffer.length(), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    

    return absl::OkStatus();
  }
};
REGISTER_CALCULATOR(PassThroughUDPCalculator);

}  // namespace mediapipe
