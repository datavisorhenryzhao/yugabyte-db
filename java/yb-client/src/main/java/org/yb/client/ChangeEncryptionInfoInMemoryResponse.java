// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

package org.yb.client;

import org.yb.master.MasterTypes;

public class ChangeEncryptionInfoInMemoryResponse extends YRpcResponse {
  // Error status indicates the tserver is not ready.
  private  MasterTypes.MasterErrorPB serverError;

  public ChangeEncryptionInfoInMemoryResponse(
          long ellapsedMillis, String uuid, MasterTypes.MasterErrorPB  error) {
    super(ellapsedMillis, uuid);
    this.serverError = error;
  }

  public boolean hasError() {
    return serverError != null;
  }

  public String getError() {
    if (serverError == null) {
      return "";
    }

    return serverError.getStatus().getMessage();
  }

}
