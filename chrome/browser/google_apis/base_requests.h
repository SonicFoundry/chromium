// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides base classes used to issue HTTP requests for Google
// APIs.

#ifndef CHROME_BROWSER_GOOGLE_APIS_BASE_REQUESTS_H_
#define CHROME_BROWSER_GOOGLE_APIS_BASE_REQUESTS_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/google_apis/gdata_errorcode.h"
#include "googleurl/src/gurl.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class Value;
}  // namespace base

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace google_apis {

class RequestSender;

// Callback used to pass parsed JSON from ParseJson(). If parsing error occurs,
// then the passed argument is null.
typedef base::Callback<void(scoped_ptr<base::Value> value)> ParseJsonCallback;

// Callback used for DownloadFileRequest and ResumeUploadRequestBase.
typedef base::Callback<void(int64 progress, int64 total)> ProgressCallback;

// Parses JSON passed in |json| on blocking pool. Runs |callback| on the calling
// thread when finished with either success or failure.
// The callback must not be null.
void ParseJson(const std::string& json, const ParseJsonCallback& callback);

//======================= AuthenticatedRequestInterface ======================

// An interface class for implementing a request which requires OAuth2
// authentication.
class AuthenticatedRequestInterface {
 public:
  // Called when re-authentication is required. See Start() for details.
  typedef base::Callback<void(AuthenticatedRequestInterface* request)>
      ReAuthenticateCallback;

  virtual ~AuthenticatedRequestInterface() {}

  // Starts the request with |access_token|. User-Agent header will be set
  // to |custom_user_agent| if the value is not empty.
  //
  // |callback| is called when re-authentication is needed for a certain
  // number of times (see kMaxReAuthenticateAttemptsPerRequest in .cc).
  // The callback should retry by calling Start() again with a new access
  // token, or just call OnAuthFailed() if a retry is not attempted.
  // |callback| must not be null.
  virtual void Start(const std::string& access_token,
                     const std::string& custom_user_agent,
                     const ReAuthenticateCallback& callback) = 0;

  // Invoked when the authentication failed with an error code |code|.
  virtual void OnAuthFailed(GDataErrorCode code) = 0;

  // Gets a weak pointer to this request object. Since requests may be
  // deleted when it is canceled by user action, for posting asynchronous tasks
  // on the authentication request object, weak pointers have to be used.
  // TODO(kinaba): crbug.com/134814 use more clean life time management than
  // using weak pointers.
  virtual base::WeakPtr<AuthenticatedRequestInterface> GetWeakPtr() = 0;

  // Cancels the request. It will invoke the callback object passed in
  // each request's constructor with error code GDATA_CANCELLED.
  virtual void Cancel() = 0;
};

//============================ UrlFetchRequestBase ===========================

// Base class for requests that are fetching URLs.
class UrlFetchRequestBase : public AuthenticatedRequestInterface,
                            public net::URLFetcherDelegate {
 public:
  // AuthenticatedRequestInterface overrides.
  virtual void Start(const std::string& access_token,
                     const std::string& custom_user_agent,
                     const ReAuthenticateCallback& callback) OVERRIDE;
  virtual base::WeakPtr<AuthenticatedRequestInterface> GetWeakPtr() OVERRIDE;
  virtual void Cancel() OVERRIDE;

 protected:
  UrlFetchRequestBase(RequestSender* sender,
                      net::URLRequestContextGetter* url_request_context_getter);
  virtual ~UrlFetchRequestBase();

  // Gets URL for the request.
  virtual GURL GetURL() const = 0;

  // Returns the request type. A derived class should override this method
  // for a request type other than HTTP GET.
  virtual net::URLFetcher::RequestType GetRequestType() const;

  // Returns the extra HTTP headers for the request. A derived class should
  // override this method to specify any extra headers needed for the request.
  virtual std::vector<std::string> GetExtraRequestHeaders() const;

  // Used by a derived class to add any content data to the request.
  // Returns true if |upload_content_type| and |upload_content| are updated
  // with the content type and data for the request.
  // Note that this and GetContentFile() cannot be used together.
  virtual bool GetContentData(std::string* upload_content_type,
                              std::string* upload_content);

  // Used by a derived class to add content data which is the whole file or
  // a part of the file at |local_file_path|.
  // Returns true if all the arguments are updated for the content being
  // uploaded.
  // Note that this and GetContentData() cannot be used together.
  virtual bool GetContentFile(base::FilePath* local_file_path,
                              int64* range_offset,
                              int64* range_length,
                              std::string* upload_content_type);

  // Invoked by OnURLFetchComplete when the request completes without an
  // authentication error. Must be implemented by a derived class.
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) = 0;

  // Invoked by this base class upon an authentication error or cancel by
  // a user request. Must be implemented by a derived class.
  virtual void RunCallbackOnPrematureFailure(GDataErrorCode code) = 0;

  // Invoked when ProcessURLFetchResults() is completed.
  void OnProcessURLFetchResultsComplete(bool result);

  // Returns an appropriate GDataErrorCode based on the HTTP response code and
  // the status of the URLFetcher.
  static GDataErrorCode GetErrorCode(const net::URLFetcher* source);

  // By default, no temporary file will be saved. Derived classes can set
  // this to true in their constructors, if they want to save the downloaded
  // content to a temporary file.
  void set_save_temp_file(bool save_temp_file) {
    save_temp_file_ = save_temp_file;
  }

  // By default, no file will be saved. Derived classes can set an output
  // file path in their constructors, if they want to save the downloaded
  // content to a file at a specific path.
  void set_output_file_path(const base::FilePath& output_file_path) {
    output_file_path_ = output_file_path;
  }

 private:
  // URLFetcherDelegate overrides.
  virtual void OnURLFetchComplete(const net::URLFetcher* source) OVERRIDE;

  // AuthenticatedRequestInterface overrides.
  virtual void OnAuthFailed(GDataErrorCode code) OVERRIDE;

  net::URLRequestContextGetter* url_request_context_getter_;
  ReAuthenticateCallback re_authenticate_callback_;
  int re_authenticate_count_;
  scoped_ptr<net::URLFetcher> url_fetcher_;
  RequestSender* sender_;

  bool save_temp_file_;
  base::FilePath output_file_path_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<UrlFetchRequestBase> weak_ptr_factory_;
};

//============================ EntryActionRequest ============================

// Callback type for Delete/Move DocumentServiceInterface calls.
typedef base::Callback<void(GDataErrorCode error)> EntryActionCallback;

// This class performs a simple action over a given entry (document/file).
// It is meant to be used for requests that return no JSON blobs.
class EntryActionRequest : public UrlFetchRequestBase {
 public:
  // |url_request_context_getter| is used to initialize URLFetcher.
  // |callback| must not be null.
  EntryActionRequest(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const EntryActionCallback& callback);
  virtual ~EntryActionRequest();

 protected:
  // Overridden from UrlFetchRequestBase.
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) OVERRIDE;
  virtual void RunCallbackOnPrematureFailure(GDataErrorCode code) OVERRIDE;

 private:
  const EntryActionCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(EntryActionRequest);
};

//============================== GetDataRequest ==============================

// Callback type for DocumentServiceInterface::GetResourceList.
// Note: json_data argument should be passed using base::Passed(&json_data), not
// json_data.Pass().
typedef base::Callback<void(GDataErrorCode error,
                            scoped_ptr<base::Value> json_data)> GetDataCallback;

// This class performs the request for fetching and converting the fetched
// content into a base::Value.
class GetDataRequest : public UrlFetchRequestBase {
 public:
  // |callback| must not be null.
  GetDataRequest(RequestSender* runner,
                 net::URLRequestContextGetter* url_request_context_getter,
                 const GetDataCallback& callback);
  virtual ~GetDataRequest();

  // Parses JSON response.
  void ParseResponse(GDataErrorCode fetch_error_code, const std::string& data);

 protected:
  // UrlFetchRequestBase overrides.
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) OVERRIDE;
  virtual void RunCallbackOnPrematureFailure(
      GDataErrorCode fetch_error_code) OVERRIDE;

 private:
  // Runs |callback_| with the given parameters.
  void RunCallbackOnSuccess(GDataErrorCode fetch_error_code,
                            scoped_ptr<base::Value> value);


  // Called when ParseJsonOnBlockingPool() is completed.
  void OnDataParsed(GDataErrorCode fetch_error_code,
                    scoped_ptr<base::Value> value);

  const GetDataCallback callback_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<GetDataRequest> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(GetDataRequest);
};


//=========================== InitiateUploadRequestBase=======================

// Callback type for DocumentServiceInterface::InitiateUpload.
typedef base::Callback<void(GDataErrorCode error,
                            const GURL& upload_url)> InitiateUploadCallback;

// This class provides base implementation for performing the request for
// initiating the upload of a file.
// |callback| will be called with the obtained upload URL. The URL will be
// used with requests for resuming the file uploading.
//
// Here's the flow of uploading:
// 1) Get the upload URL with a class inheriting InitiateUploadRequestBase.
// 2) Upload the first 512KB (see kUploadChunkSize in drive_uploader.cc)
//    of the target file to the upload URL
// 3) If there is more data to upload, go to 2).
//
class InitiateUploadRequestBase : public UrlFetchRequestBase {
 protected:
  // |callback| will be called with the upload URL, where upload data is
  // uploaded to with ResumeUploadRequestBase.
  // |callback| must not be null.
  // |content_type| and |content_length| should be the attributes of the
  // uploading file.
  InitiateUploadRequestBase(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const InitiateUploadCallback& callback,
      const std::string& content_type,
      int64 content_length);
  virtual ~InitiateUploadRequestBase();

  // UrlFetchRequestBase overrides.
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) OVERRIDE;
  virtual void RunCallbackOnPrematureFailure(GDataErrorCode code) OVERRIDE;
  virtual std::vector<std::string> GetExtraRequestHeaders() const OVERRIDE;

 private:
  const InitiateUploadCallback callback_;
  const std::string content_type_;
  const int64 content_length_;

  DISALLOW_COPY_AND_ASSIGN(InitiateUploadRequestBase);
};

//========================== UploadRangeRequestBase ==========================

// Struct for response to ResumeUpload and GetUploadStatus.
struct UploadRangeResponse {
  UploadRangeResponse();
  UploadRangeResponse(GDataErrorCode code,
                      int64 start_position_received,
                      int64 end_position_received);
  ~UploadRangeResponse();

  GDataErrorCode code;
  // The values of "Range" header returned from the server. The values are
  // used to continue uploading more data. These are set to -1 if an upload
  // is complete.
  // |start_position_received| is inclusive and |end_position_received| is
  // exclusive to follow the common C++ manner, although the response from
  // the server has "Range" header in inclusive format at both sides.
  int64 start_position_received;
  int64 end_position_received;
};

// Base class for a URL fetch request expecting the response containing the
// current uploading range. This class processes the response containing
// "Range" header and invoke OnRangeRequestComplete.
class UploadRangeRequestBase : public UrlFetchRequestBase {
 protected:
  // |upload_location| is the URL of where to upload the file to.
  UploadRangeRequestBase(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const GURL& upload_url);
  virtual ~UploadRangeRequestBase();

  // UrlFetchRequestBase overrides.
  virtual GURL GetURL() const OVERRIDE;
  virtual net::URLFetcher::RequestType GetRequestType() const OVERRIDE;
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) OVERRIDE;
  virtual void RunCallbackOnPrematureFailure(GDataErrorCode code) OVERRIDE;

  // This method will be called when the request is done, regardless of
  // whether it is succeeded or failed.
  //
  // 1) If there is more data to upload, |code| of |response| is set to
  // HTTP_RESUME_INCOMPLETE, and positions are set appropriately. Also, |value|
  // will be set to NULL.
  // 2) If the upload is complete, |code| is set to HTTP_CREATED for a new file
  // or HTTP_SUCCESS for an existing file. Positions are set to -1, and |value|
  // is set to a parsed JSON value representing the uploaded file.
  // 3) If a premature failure is found, |code| is set to a value representing
  // the situation. Positions are set to 0, and |value| is set to NULL.
  //
  // See also the comments for UploadRangeResponse.
  // Note: Subclasses should have responsibility to run some callback
  // in this method to notify the finish status to its clients (or ignore it
  // under its responsibility).
  virtual void OnRangeRequestComplete(
      const UploadRangeResponse& response, scoped_ptr<base::Value> value) = 0;

 private:
  // Called when ParseJson() is completed.
  void OnDataParsed(GDataErrorCode code, scoped_ptr<base::Value> value);

  const GURL upload_url_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<UploadRangeRequestBase> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(UploadRangeRequestBase);
};

//========================== ResumeUploadRequestBase =========================

// This class performs the request for resuming the upload of a file.
// More specifically, this request uploads a chunk of data carried in |buf|
// of ResumeUploadResponseBase. This class is designed to share the
// implementation of upload resuming between GData WAPI and Drive API v2.
// The subclasses should implement OnRangeRequestComplete inherited by
// UploadRangeRequestBase, because the type of the response should be
// different (although the format in the server response is JSON).
class ResumeUploadRequestBase : public UploadRangeRequestBase {
 protected:
  // |start_position| is the start of range of contents currently stored in
  // |buf|. |end_position| is the end of range of contents currently stared in
  // |buf|. This is exclusive. For instance, if you are to upload the first
  // 500 bytes of data, |start_position| is 0 and |end_position| is 500.
  // |content_length| and |content_type| are the length and type of the
  // file content to be uploaded respectively.
  // |buf| holds current content to be uploaded.
  // See also UploadRangeRequestBase's comment for remaining parameters
  // meaning.
  ResumeUploadRequestBase(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const GURL& upload_location,
      int64 start_position,
      int64 end_position,
      int64 content_length,
      const std::string& content_type,
      const base::FilePath& local_file_path);
  virtual ~ResumeUploadRequestBase();

  // UrlFetchRequestBase overrides.
  virtual std::vector<std::string> GetExtraRequestHeaders() const OVERRIDE;
  virtual bool GetContentFile(base::FilePath* local_file_path,
                              int64* range_offset,
                              int64* range_length,
                              std::string* upload_content_type) OVERRIDE;

 private:
  // The parameters for the request. See ResumeUploadParams for the details.
  const int64 start_position_;
  const int64 end_position_;
  const int64 content_length_;
  const std::string content_type_;
  const base::FilePath local_file_path_;

  DISALLOW_COPY_AND_ASSIGN(ResumeUploadRequestBase);
};

//======================== GetUploadStatusRequestBase ========================

// This class performs the request for getting the current upload status
// of a file.
// This request calls OnRangeRequestComplete() with:
// - HTTP_RESUME_INCOMPLETE and the range of previously uploaded data,
//   if a file has been partially uploaded. |value| is not used.
// - HTTP_SUCCESS or HTTP_CREATED (up to the upload mode) and |value|
//   for the uploaded data, if a file has been completely uploaded.
// See also UploadRangeRequestBase.
class GetUploadStatusRequestBase : public UploadRangeRequestBase {
 public:
  // |content_length| is the whole data size to be uploaded.
  // See also UploadRangeRequestBase's constructor comment for other
  // parameters.
  GetUploadStatusRequestBase(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const GURL& upload_url,
      int64 content_length);
  virtual ~GetUploadStatusRequestBase();

 protected:
  // UrlFetchRequestBase overrides.
  virtual std::vector<std::string> GetExtraRequestHeaders() const OVERRIDE;

 private:
  const int64 content_length_;

  DISALLOW_COPY_AND_ASSIGN(GetUploadStatusRequestBase);
};

//============================ DownloadFileRequest ===========================

// Callback type for getting the content from DownloadFileRequest.
typedef base::Callback<void(
    GDataErrorCode error,
    scoped_ptr<std::string> content)> GetContentCallback;

// Callback type for receiving the completion of DownloadFileRequest.
typedef base::Callback<void(GDataErrorCode error,
                            const base::FilePath& temp_file)>
    DownloadActionCallback;

// This class performs the request for downloading of a given document/file.
class DownloadFileRequest : public UrlFetchRequestBase {
 public:
  // download_action_callback:
  //   This callback is called when the download is complete. Must not be null.
  //
  // get_content_callback:
  //   This callback is called when some part of the content is
  //   read. Used to read the download content progressively. May be null.
  //
  // progress_callback:
  //   This callback is called for periodically reporting the number of bytes
  //   downloaded so far. May be null.
  //
  // download_url:
  //   Specifies the target file to download.
  //
  // output_file_path:
  //   Specifies the file path to save the downloaded file.
  //
  DownloadFileRequest(
      RequestSender* runner,
      net::URLRequestContextGetter* url_request_context_getter,
      const DownloadActionCallback& download_action_callback,
      const GetContentCallback& get_content_callback,
      const ProgressCallback& progress_callback,
      const GURL& download_url,
      const base::FilePath& output_file_path);
  virtual ~DownloadFileRequest();

 protected:
  // UrlFetchRequestBase overrides.
  virtual GURL GetURL() const OVERRIDE;
  virtual void ProcessURLFetchResults(const net::URLFetcher* source) OVERRIDE;
  virtual void RunCallbackOnPrematureFailure(GDataErrorCode code) OVERRIDE;

  // net::URLFetcherDelegate overrides.
  virtual void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                          int64 current, int64 total) OVERRIDE;
  virtual bool ShouldSendDownloadData() OVERRIDE;
  virtual void OnURLFetchDownloadData(
      const net::URLFetcher* source,
      scoped_ptr<std::string> download_data) OVERRIDE;

 private:
  const DownloadActionCallback download_action_callback_;
  const GetContentCallback get_content_callback_;
  const ProgressCallback progress_callback_;
  const GURL download_url_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFileRequest);
};

}  // namespace google_apis

#endif  // CHROME_BROWSER_GOOGLE_APIS_BASE_REQUESTS_H_
