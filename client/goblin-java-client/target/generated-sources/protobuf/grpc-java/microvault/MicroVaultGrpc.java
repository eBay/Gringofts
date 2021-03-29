package microvault;

import static io.grpc.MethodDescriptor.generateFullMethodName;
import static io.grpc.stub.ClientCalls.asyncBidiStreamingCall;
import static io.grpc.stub.ClientCalls.asyncClientStreamingCall;
import static io.grpc.stub.ClientCalls.asyncServerStreamingCall;
import static io.grpc.stub.ClientCalls.asyncUnaryCall;
import static io.grpc.stub.ClientCalls.blockingServerStreamingCall;
import static io.grpc.stub.ClientCalls.blockingUnaryCall;
import static io.grpc.stub.ClientCalls.futureUnaryCall;
import static io.grpc.stub.ServerCalls.asyncBidiStreamingCall;
import static io.grpc.stub.ServerCalls.asyncClientStreamingCall;
import static io.grpc.stub.ServerCalls.asyncServerStreamingCall;
import static io.grpc.stub.ServerCalls.asyncUnaryCall;
import static io.grpc.stub.ServerCalls.asyncUnimplementedStreamingCall;
import static io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall;

/**
 */
@javax.annotation.Generated(
    value = "by gRPC proto compiler (version 1.23.0)",
    comments = "Source: microvault.proto")
public final class MicroVaultGrpc {

  private MicroVaultGrpc() {}

  public static final String SERVICE_NAME = "microvault.MicroVault";

  // Static method descriptors that strictly reflect the proto.
  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.GetSecretRequest,
      microvault.Microvault.GetSecretResponse> getGetSecretMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "GetSecret",
      requestType = microvault.Microvault.GetSecretRequest.class,
      responseType = microvault.Microvault.GetSecretResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.GetSecretRequest,
      microvault.Microvault.GetSecretResponse> getGetSecretMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.GetSecretRequest, microvault.Microvault.GetSecretResponse> getGetSecretMethod;
    if ((getGetSecretMethod = MicroVaultGrpc.getGetSecretMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getGetSecretMethod = MicroVaultGrpc.getGetSecretMethod) == null) {
          MicroVaultGrpc.getGetSecretMethod = getGetSecretMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.GetSecretRequest, microvault.Microvault.GetSecretResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "GetSecret"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetSecretRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetSecretResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("GetSecret"))
              .build();
        }
      }
    }
    return getGetSecretMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.GetKeyInfoRequest,
      microvault.Microvault.GetKeyInfoResponse> getGetKeyInfoMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "GetKeyInfo",
      requestType = microvault.Microvault.GetKeyInfoRequest.class,
      responseType = microvault.Microvault.GetKeyInfoResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.GetKeyInfoRequest,
      microvault.Microvault.GetKeyInfoResponse> getGetKeyInfoMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.GetKeyInfoRequest, microvault.Microvault.GetKeyInfoResponse> getGetKeyInfoMethod;
    if ((getGetKeyInfoMethod = MicroVaultGrpc.getGetKeyInfoMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getGetKeyInfoMethod = MicroVaultGrpc.getGetKeyInfoMethod) == null) {
          MicroVaultGrpc.getGetKeyInfoMethod = getGetKeyInfoMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.GetKeyInfoRequest, microvault.Microvault.GetKeyInfoResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "GetKeyInfo"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetKeyInfoRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetKeyInfoResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("GetKeyInfo"))
              .build();
        }
      }
    }
    return getGetKeyInfoMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.ExportKeyRequest,
      microvault.Microvault.ExportKeyResponse> getExportKeyMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "ExportKey",
      requestType = microvault.Microvault.ExportKeyRequest.class,
      responseType = microvault.Microvault.ExportKeyResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.ExportKeyRequest,
      microvault.Microvault.ExportKeyResponse> getExportKeyMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.ExportKeyRequest, microvault.Microvault.ExportKeyResponse> getExportKeyMethod;
    if ((getExportKeyMethod = MicroVaultGrpc.getExportKeyMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getExportKeyMethod = MicroVaultGrpc.getExportKeyMethod) == null) {
          MicroVaultGrpc.getExportKeyMethod = getExportKeyMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.ExportKeyRequest, microvault.Microvault.ExportKeyResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "ExportKey"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.ExportKeyRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.ExportKeyResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("ExportKey"))
              .build();
        }
      }
    }
    return getExportKeyMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.EncryptRequest,
      microvault.Microvault.EncryptResponse> getEncryptMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Encrypt",
      requestType = microvault.Microvault.EncryptRequest.class,
      responseType = microvault.Microvault.EncryptResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.EncryptRequest,
      microvault.Microvault.EncryptResponse> getEncryptMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.EncryptRequest, microvault.Microvault.EncryptResponse> getEncryptMethod;
    if ((getEncryptMethod = MicroVaultGrpc.getEncryptMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getEncryptMethod = MicroVaultGrpc.getEncryptMethod) == null) {
          MicroVaultGrpc.getEncryptMethod = getEncryptMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.EncryptRequest, microvault.Microvault.EncryptResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Encrypt"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.EncryptRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.EncryptResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Encrypt"))
              .build();
        }
      }
    }
    return getEncryptMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.DecryptRequest,
      microvault.Microvault.DecryptResponse> getDecryptMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Decrypt",
      requestType = microvault.Microvault.DecryptRequest.class,
      responseType = microvault.Microvault.DecryptResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.DecryptRequest,
      microvault.Microvault.DecryptResponse> getDecryptMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.DecryptRequest, microvault.Microvault.DecryptResponse> getDecryptMethod;
    if ((getDecryptMethod = MicroVaultGrpc.getDecryptMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getDecryptMethod = MicroVaultGrpc.getDecryptMethod) == null) {
          MicroVaultGrpc.getDecryptMethod = getDecryptMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.DecryptRequest, microvault.Microvault.DecryptResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Decrypt"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.DecryptRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.DecryptResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Decrypt"))
              .build();
        }
      }
    }
    return getDecryptMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.SignRequest,
      microvault.Microvault.SignResponse> getSignMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Sign",
      requestType = microvault.Microvault.SignRequest.class,
      responseType = microvault.Microvault.SignResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.SignRequest,
      microvault.Microvault.SignResponse> getSignMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.SignRequest, microvault.Microvault.SignResponse> getSignMethod;
    if ((getSignMethod = MicroVaultGrpc.getSignMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getSignMethod = MicroVaultGrpc.getSignMethod) == null) {
          MicroVaultGrpc.getSignMethod = getSignMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.SignRequest, microvault.Microvault.SignResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Sign"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SignRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SignResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Sign"))
              .build();
        }
      }
    }
    return getSignMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.VerifyRequest,
      microvault.Microvault.VerifyResponse> getVerifyMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Verify",
      requestType = microvault.Microvault.VerifyRequest.class,
      responseType = microvault.Microvault.VerifyResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.VerifyRequest,
      microvault.Microvault.VerifyResponse> getVerifyMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.VerifyRequest, microvault.Microvault.VerifyResponse> getVerifyMethod;
    if ((getVerifyMethod = MicroVaultGrpc.getVerifyMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getVerifyMethod = MicroVaultGrpc.getVerifyMethod) == null) {
          MicroVaultGrpc.getVerifyMethod = getVerifyMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.VerifyRequest, microvault.Microvault.VerifyResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Verify"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.VerifyRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.VerifyResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Verify"))
              .build();
        }
      }
    }
    return getVerifyMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.SealRequest,
      microvault.Microvault.SealResponse> getSealMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Seal",
      requestType = microvault.Microvault.SealRequest.class,
      responseType = microvault.Microvault.SealResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.SealRequest,
      microvault.Microvault.SealResponse> getSealMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.SealRequest, microvault.Microvault.SealResponse> getSealMethod;
    if ((getSealMethod = MicroVaultGrpc.getSealMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getSealMethod = MicroVaultGrpc.getSealMethod) == null) {
          MicroVaultGrpc.getSealMethod = getSealMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.SealRequest, microvault.Microvault.SealResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Seal"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SealRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SealResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Seal"))
              .build();
        }
      }
    }
    return getSealMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.UnsealRequest,
      microvault.Microvault.UnsealResponse> getUnsealMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Unseal",
      requestType = microvault.Microvault.UnsealRequest.class,
      responseType = microvault.Microvault.UnsealResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.UnsealRequest,
      microvault.Microvault.UnsealResponse> getUnsealMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.UnsealRequest, microvault.Microvault.UnsealResponse> getUnsealMethod;
    if ((getUnsealMethod = MicroVaultGrpc.getUnsealMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getUnsealMethod = MicroVaultGrpc.getUnsealMethod) == null) {
          MicroVaultGrpc.getUnsealMethod = getUnsealMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.UnsealRequest, microvault.Microvault.UnsealResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Unseal"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.UnsealRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.UnsealResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("Unseal"))
              .build();
        }
      }
    }
    return getUnsealMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.SealStreamRequest,
      microvault.Microvault.SealStreamResponse> getSealStreamMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "SealStream",
      requestType = microvault.Microvault.SealStreamRequest.class,
      responseType = microvault.Microvault.SealStreamResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
  public static io.grpc.MethodDescriptor<microvault.Microvault.SealStreamRequest,
      microvault.Microvault.SealStreamResponse> getSealStreamMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.SealStreamRequest, microvault.Microvault.SealStreamResponse> getSealStreamMethod;
    if ((getSealStreamMethod = MicroVaultGrpc.getSealStreamMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getSealStreamMethod = MicroVaultGrpc.getSealStreamMethod) == null) {
          MicroVaultGrpc.getSealStreamMethod = getSealStreamMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.SealStreamRequest, microvault.Microvault.SealStreamResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "SealStream"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SealStreamRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.SealStreamResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("SealStream"))
              .build();
        }
      }
    }
    return getSealStreamMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.UnsealStreamRequest,
      microvault.Microvault.UnsealStreamResponse> getUnsealStreamMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "UnsealStream",
      requestType = microvault.Microvault.UnsealStreamRequest.class,
      responseType = microvault.Microvault.UnsealStreamResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
  public static io.grpc.MethodDescriptor<microvault.Microvault.UnsealStreamRequest,
      microvault.Microvault.UnsealStreamResponse> getUnsealStreamMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.UnsealStreamRequest, microvault.Microvault.UnsealStreamResponse> getUnsealStreamMethod;
    if ((getUnsealStreamMethod = MicroVaultGrpc.getUnsealStreamMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getUnsealStreamMethod = MicroVaultGrpc.getUnsealStreamMethod) == null) {
          MicroVaultGrpc.getUnsealStreamMethod = getUnsealStreamMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.UnsealStreamRequest, microvault.Microvault.UnsealStreamResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "UnsealStream"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.UnsealStreamRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.UnsealStreamResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("UnsealStream"))
              .build();
        }
      }
    }
    return getUnsealStreamMethod;
  }

  private static volatile io.grpc.MethodDescriptor<microvault.Microvault.GetCertRequest,
      microvault.Microvault.GetCertResponse> getGetCertMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "GetCert",
      requestType = microvault.Microvault.GetCertRequest.class,
      responseType = microvault.Microvault.GetCertResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<microvault.Microvault.GetCertRequest,
      microvault.Microvault.GetCertResponse> getGetCertMethod() {
    io.grpc.MethodDescriptor<microvault.Microvault.GetCertRequest, microvault.Microvault.GetCertResponse> getGetCertMethod;
    if ((getGetCertMethod = MicroVaultGrpc.getGetCertMethod) == null) {
      synchronized (MicroVaultGrpc.class) {
        if ((getGetCertMethod = MicroVaultGrpc.getGetCertMethod) == null) {
          MicroVaultGrpc.getGetCertMethod = getGetCertMethod =
              io.grpc.MethodDescriptor.<microvault.Microvault.GetCertRequest, microvault.Microvault.GetCertResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "GetCert"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetCertRequest.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  microvault.Microvault.GetCertResponse.getDefaultInstance()))
              .setSchemaDescriptor(new MicroVaultMethodDescriptorSupplier("GetCert"))
              .build();
        }
      }
    }
    return getGetCertMethod;
  }

  /**
   * Creates a new async stub that supports all call types for the service
   */
  public static MicroVaultStub newStub(io.grpc.Channel channel) {
    return new MicroVaultStub(channel);
  }

  /**
   * Creates a new blocking-style stub that supports unary and streaming output calls on the service
   */
  public static MicroVaultBlockingStub newBlockingStub(
      io.grpc.Channel channel) {
    return new MicroVaultBlockingStub(channel);
  }

  /**
   * Creates a new ListenableFuture-style stub that supports unary calls on the service
   */
  public static MicroVaultFutureStub newFutureStub(
      io.grpc.Channel channel) {
    return new MicroVaultFutureStub(channel);
  }

  /**
   */
  public static abstract class MicroVaultImplBase implements io.grpc.BindableService {

    /**
     */
    public void getSecret(microvault.Microvault.GetSecretRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetSecretResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getGetSecretMethod(), responseObserver);
    }

    /**
     */
    public void getKeyInfo(microvault.Microvault.GetKeyInfoRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetKeyInfoResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getGetKeyInfoMethod(), responseObserver);
    }

    /**
     */
    public void exportKey(microvault.Microvault.ExportKeyRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.ExportKeyResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getExportKeyMethod(), responseObserver);
    }

    /**
     */
    public void encrypt(microvault.Microvault.EncryptRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.EncryptResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getEncryptMethod(), responseObserver);
    }

    /**
     */
    public void decrypt(microvault.Microvault.DecryptRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.DecryptResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getDecryptMethod(), responseObserver);
    }

    /**
     */
    public void sign(microvault.Microvault.SignRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.SignResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getSignMethod(), responseObserver);
    }

    /**
     */
    public void verify(microvault.Microvault.VerifyRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.VerifyResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getVerifyMethod(), responseObserver);
    }

    /**
     */
    public void seal(microvault.Microvault.SealRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.SealResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getSealMethod(), responseObserver);
    }

    /**
     */
    public void unseal(microvault.Microvault.UnsealRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.UnsealResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getUnsealMethod(), responseObserver);
    }

    /**
     */
    public io.grpc.stub.StreamObserver<microvault.Microvault.SealStreamRequest> sealStream(
        io.grpc.stub.StreamObserver<microvault.Microvault.SealStreamResponse> responseObserver) {
      return asyncUnimplementedStreamingCall(getSealStreamMethod(), responseObserver);
    }

    /**
     */
    public io.grpc.stub.StreamObserver<microvault.Microvault.UnsealStreamRequest> unsealStream(
        io.grpc.stub.StreamObserver<microvault.Microvault.UnsealStreamResponse> responseObserver) {
      return asyncUnimplementedStreamingCall(getUnsealStreamMethod(), responseObserver);
    }

    /**
     */
    public void getCert(microvault.Microvault.GetCertRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetCertResponse> responseObserver) {
      asyncUnimplementedUnaryCall(getGetCertMethod(), responseObserver);
    }

    @java.lang.Override public final io.grpc.ServerServiceDefinition bindService() {
      return io.grpc.ServerServiceDefinition.builder(getServiceDescriptor())
          .addMethod(
            getGetSecretMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.GetSecretRequest,
                microvault.Microvault.GetSecretResponse>(
                  this, METHODID_GET_SECRET)))
          .addMethod(
            getGetKeyInfoMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.GetKeyInfoRequest,
                microvault.Microvault.GetKeyInfoResponse>(
                  this, METHODID_GET_KEY_INFO)))
          .addMethod(
            getExportKeyMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.ExportKeyRequest,
                microvault.Microvault.ExportKeyResponse>(
                  this, METHODID_EXPORT_KEY)))
          .addMethod(
            getEncryptMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.EncryptRequest,
                microvault.Microvault.EncryptResponse>(
                  this, METHODID_ENCRYPT)))
          .addMethod(
            getDecryptMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.DecryptRequest,
                microvault.Microvault.DecryptResponse>(
                  this, METHODID_DECRYPT)))
          .addMethod(
            getSignMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.SignRequest,
                microvault.Microvault.SignResponse>(
                  this, METHODID_SIGN)))
          .addMethod(
            getVerifyMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.VerifyRequest,
                microvault.Microvault.VerifyResponse>(
                  this, METHODID_VERIFY)))
          .addMethod(
            getSealMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.SealRequest,
                microvault.Microvault.SealResponse>(
                  this, METHODID_SEAL)))
          .addMethod(
            getUnsealMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.UnsealRequest,
                microvault.Microvault.UnsealResponse>(
                  this, METHODID_UNSEAL)))
          .addMethod(
            getSealStreamMethod(),
            asyncBidiStreamingCall(
              new MethodHandlers<
                microvault.Microvault.SealStreamRequest,
                microvault.Microvault.SealStreamResponse>(
                  this, METHODID_SEAL_STREAM)))
          .addMethod(
            getUnsealStreamMethod(),
            asyncBidiStreamingCall(
              new MethodHandlers<
                microvault.Microvault.UnsealStreamRequest,
                microvault.Microvault.UnsealStreamResponse>(
                  this, METHODID_UNSEAL_STREAM)))
          .addMethod(
            getGetCertMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                microvault.Microvault.GetCertRequest,
                microvault.Microvault.GetCertResponse>(
                  this, METHODID_GET_CERT)))
          .build();
    }
  }

  /**
   */
  public static final class MicroVaultStub extends io.grpc.stub.AbstractStub<MicroVaultStub> {
    private MicroVaultStub(io.grpc.Channel channel) {
      super(channel);
    }

    private MicroVaultStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected MicroVaultStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new MicroVaultStub(channel, callOptions);
    }

    /**
     */
    public void getSecret(microvault.Microvault.GetSecretRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetSecretResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getGetSecretMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void getKeyInfo(microvault.Microvault.GetKeyInfoRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetKeyInfoResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getGetKeyInfoMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void exportKey(microvault.Microvault.ExportKeyRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.ExportKeyResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getExportKeyMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void encrypt(microvault.Microvault.EncryptRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.EncryptResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getEncryptMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void decrypt(microvault.Microvault.DecryptRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.DecryptResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getDecryptMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void sign(microvault.Microvault.SignRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.SignResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getSignMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void verify(microvault.Microvault.VerifyRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.VerifyResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getVerifyMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void seal(microvault.Microvault.SealRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.SealResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getSealMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void unseal(microvault.Microvault.UnsealRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.UnsealResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getUnsealMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public io.grpc.stub.StreamObserver<microvault.Microvault.SealStreamRequest> sealStream(
        io.grpc.stub.StreamObserver<microvault.Microvault.SealStreamResponse> responseObserver) {
      return asyncBidiStreamingCall(
          getChannel().newCall(getSealStreamMethod(), getCallOptions()), responseObserver);
    }

    /**
     */
    public io.grpc.stub.StreamObserver<microvault.Microvault.UnsealStreamRequest> unsealStream(
        io.grpc.stub.StreamObserver<microvault.Microvault.UnsealStreamResponse> responseObserver) {
      return asyncBidiStreamingCall(
          getChannel().newCall(getUnsealStreamMethod(), getCallOptions()), responseObserver);
    }

    /**
     */
    public void getCert(microvault.Microvault.GetCertRequest request,
        io.grpc.stub.StreamObserver<microvault.Microvault.GetCertResponse> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getGetCertMethod(), getCallOptions()), request, responseObserver);
    }
  }

  /**
   */
  public static final class MicroVaultBlockingStub extends io.grpc.stub.AbstractStub<MicroVaultBlockingStub> {
    private MicroVaultBlockingStub(io.grpc.Channel channel) {
      super(channel);
    }

    private MicroVaultBlockingStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected MicroVaultBlockingStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new MicroVaultBlockingStub(channel, callOptions);
    }

    /**
     */
    public microvault.Microvault.GetSecretResponse getSecret(microvault.Microvault.GetSecretRequest request) {
      return blockingUnaryCall(
          getChannel(), getGetSecretMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.GetKeyInfoResponse getKeyInfo(microvault.Microvault.GetKeyInfoRequest request) {
      return blockingUnaryCall(
          getChannel(), getGetKeyInfoMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.ExportKeyResponse exportKey(microvault.Microvault.ExportKeyRequest request) {
      return blockingUnaryCall(
          getChannel(), getExportKeyMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.EncryptResponse encrypt(microvault.Microvault.EncryptRequest request) {
      return blockingUnaryCall(
          getChannel(), getEncryptMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.DecryptResponse decrypt(microvault.Microvault.DecryptRequest request) {
      return blockingUnaryCall(
          getChannel(), getDecryptMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.SignResponse sign(microvault.Microvault.SignRequest request) {
      return blockingUnaryCall(
          getChannel(), getSignMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.VerifyResponse verify(microvault.Microvault.VerifyRequest request) {
      return blockingUnaryCall(
          getChannel(), getVerifyMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.SealResponse seal(microvault.Microvault.SealRequest request) {
      return blockingUnaryCall(
          getChannel(), getSealMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.UnsealResponse unseal(microvault.Microvault.UnsealRequest request) {
      return blockingUnaryCall(
          getChannel(), getUnsealMethod(), getCallOptions(), request);
    }

    /**
     */
    public microvault.Microvault.GetCertResponse getCert(microvault.Microvault.GetCertRequest request) {
      return blockingUnaryCall(
          getChannel(), getGetCertMethod(), getCallOptions(), request);
    }
  }

  /**
   */
  public static final class MicroVaultFutureStub extends io.grpc.stub.AbstractStub<MicroVaultFutureStub> {
    private MicroVaultFutureStub(io.grpc.Channel channel) {
      super(channel);
    }

    private MicroVaultFutureStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected MicroVaultFutureStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new MicroVaultFutureStub(channel, callOptions);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.GetSecretResponse> getSecret(
        microvault.Microvault.GetSecretRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getGetSecretMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.GetKeyInfoResponse> getKeyInfo(
        microvault.Microvault.GetKeyInfoRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getGetKeyInfoMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.ExportKeyResponse> exportKey(
        microvault.Microvault.ExportKeyRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getExportKeyMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.EncryptResponse> encrypt(
        microvault.Microvault.EncryptRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getEncryptMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.DecryptResponse> decrypt(
        microvault.Microvault.DecryptRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getDecryptMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.SignResponse> sign(
        microvault.Microvault.SignRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getSignMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.VerifyResponse> verify(
        microvault.Microvault.VerifyRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getVerifyMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.SealResponse> seal(
        microvault.Microvault.SealRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getSealMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.UnsealResponse> unseal(
        microvault.Microvault.UnsealRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getUnsealMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<microvault.Microvault.GetCertResponse> getCert(
        microvault.Microvault.GetCertRequest request) {
      return futureUnaryCall(
          getChannel().newCall(getGetCertMethod(), getCallOptions()), request);
    }
  }

  private static final int METHODID_GET_SECRET = 0;
  private static final int METHODID_GET_KEY_INFO = 1;
  private static final int METHODID_EXPORT_KEY = 2;
  private static final int METHODID_ENCRYPT = 3;
  private static final int METHODID_DECRYPT = 4;
  private static final int METHODID_SIGN = 5;
  private static final int METHODID_VERIFY = 6;
  private static final int METHODID_SEAL = 7;
  private static final int METHODID_UNSEAL = 8;
  private static final int METHODID_GET_CERT = 9;
  private static final int METHODID_SEAL_STREAM = 10;
  private static final int METHODID_UNSEAL_STREAM = 11;

  private static final class MethodHandlers<Req, Resp> implements
      io.grpc.stub.ServerCalls.UnaryMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ServerStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ClientStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.BidiStreamingMethod<Req, Resp> {
    private final MicroVaultImplBase serviceImpl;
    private final int methodId;

    MethodHandlers(MicroVaultImplBase serviceImpl, int methodId) {
      this.serviceImpl = serviceImpl;
      this.methodId = methodId;
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public void invoke(Req request, io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_GET_SECRET:
          serviceImpl.getSecret((microvault.Microvault.GetSecretRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.GetSecretResponse>) responseObserver);
          break;
        case METHODID_GET_KEY_INFO:
          serviceImpl.getKeyInfo((microvault.Microvault.GetKeyInfoRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.GetKeyInfoResponse>) responseObserver);
          break;
        case METHODID_EXPORT_KEY:
          serviceImpl.exportKey((microvault.Microvault.ExportKeyRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.ExportKeyResponse>) responseObserver);
          break;
        case METHODID_ENCRYPT:
          serviceImpl.encrypt((microvault.Microvault.EncryptRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.EncryptResponse>) responseObserver);
          break;
        case METHODID_DECRYPT:
          serviceImpl.decrypt((microvault.Microvault.DecryptRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.DecryptResponse>) responseObserver);
          break;
        case METHODID_SIGN:
          serviceImpl.sign((microvault.Microvault.SignRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.SignResponse>) responseObserver);
          break;
        case METHODID_VERIFY:
          serviceImpl.verify((microvault.Microvault.VerifyRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.VerifyResponse>) responseObserver);
          break;
        case METHODID_SEAL:
          serviceImpl.seal((microvault.Microvault.SealRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.SealResponse>) responseObserver);
          break;
        case METHODID_UNSEAL:
          serviceImpl.unseal((microvault.Microvault.UnsealRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.UnsealResponse>) responseObserver);
          break;
        case METHODID_GET_CERT:
          serviceImpl.getCert((microvault.Microvault.GetCertRequest) request,
              (io.grpc.stub.StreamObserver<microvault.Microvault.GetCertResponse>) responseObserver);
          break;
        default:
          throw new AssertionError();
      }
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public io.grpc.stub.StreamObserver<Req> invoke(
        io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_SEAL_STREAM:
          return (io.grpc.stub.StreamObserver<Req>) serviceImpl.sealStream(
              (io.grpc.stub.StreamObserver<microvault.Microvault.SealStreamResponse>) responseObserver);
        case METHODID_UNSEAL_STREAM:
          return (io.grpc.stub.StreamObserver<Req>) serviceImpl.unsealStream(
              (io.grpc.stub.StreamObserver<microvault.Microvault.UnsealStreamResponse>) responseObserver);
        default:
          throw new AssertionError();
      }
    }
  }

  private static abstract class MicroVaultBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoFileDescriptorSupplier, io.grpc.protobuf.ProtoServiceDescriptorSupplier {
    MicroVaultBaseDescriptorSupplier() {}

    @java.lang.Override
    public com.google.protobuf.Descriptors.FileDescriptor getFileDescriptor() {
      return microvault.Microvault.getDescriptor();
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.ServiceDescriptor getServiceDescriptor() {
      return getFileDescriptor().findServiceByName("MicroVault");
    }
  }

  private static final class MicroVaultFileDescriptorSupplier
      extends MicroVaultBaseDescriptorSupplier {
    MicroVaultFileDescriptorSupplier() {}
  }

  private static final class MicroVaultMethodDescriptorSupplier
      extends MicroVaultBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoMethodDescriptorSupplier {
    private final String methodName;

    MicroVaultMethodDescriptorSupplier(String methodName) {
      this.methodName = methodName;
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.MethodDescriptor getMethodDescriptor() {
      return getServiceDescriptor().findMethodByName(methodName);
    }
  }

  private static volatile io.grpc.ServiceDescriptor serviceDescriptor;

  public static io.grpc.ServiceDescriptor getServiceDescriptor() {
    io.grpc.ServiceDescriptor result = serviceDescriptor;
    if (result == null) {
      synchronized (MicroVaultGrpc.class) {
        result = serviceDescriptor;
        if (result == null) {
          serviceDescriptor = result = io.grpc.ServiceDescriptor.newBuilder(SERVICE_NAME)
              .setSchemaDescriptor(new MicroVaultFileDescriptorSupplier())
              .addMethod(getGetSecretMethod())
              .addMethod(getGetKeyInfoMethod())
              .addMethod(getExportKeyMethod())
              .addMethod(getEncryptMethod())
              .addMethod(getDecryptMethod())
              .addMethod(getSignMethod())
              .addMethod(getVerifyMethod())
              .addMethod(getSealMethod())
              .addMethod(getUnsealMethod())
              .addMethod(getSealStreamMethod())
              .addMethod(getUnsealStreamMethod())
              .addMethod(getGetCertMethod())
              .build();
        }
      }
    }
    return result;
  }
}
