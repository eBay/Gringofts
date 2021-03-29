package goblin.proto;

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
    comments = "Source: service.proto")
public final class KVStoreGrpc {

  private KVStoreGrpc() {}

  public static final String SERVICE_NAME = "goblin.proto.KVStore";

  // Static method descriptors that strictly reflect the proto.
  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Connect.Request,
      goblin.proto.Service.Connect.Response> getConnectMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Connect",
      requestType = goblin.proto.Service.Connect.Request.class,
      responseType = goblin.proto.Service.Connect.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Connect.Request,
      goblin.proto.Service.Connect.Response> getConnectMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Connect.Request, goblin.proto.Service.Connect.Response> getConnectMethod;
    if ((getConnectMethod = KVStoreGrpc.getConnectMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getConnectMethod = KVStoreGrpc.getConnectMethod) == null) {
          KVStoreGrpc.getConnectMethod = getConnectMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Connect.Request, goblin.proto.Service.Connect.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Connect"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Connect.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Connect.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Connect"))
              .build();
        }
      }
    }
    return getConnectMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Put.Request,
      goblin.proto.Service.Put.Response> getPutMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Put",
      requestType = goblin.proto.Service.Put.Request.class,
      responseType = goblin.proto.Service.Put.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Put.Request,
      goblin.proto.Service.Put.Response> getPutMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Put.Request, goblin.proto.Service.Put.Response> getPutMethod;
    if ((getPutMethod = KVStoreGrpc.getPutMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getPutMethod = KVStoreGrpc.getPutMethod) == null) {
          KVStoreGrpc.getPutMethod = getPutMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Put.Request, goblin.proto.Service.Put.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Put"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Put.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Put.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Put"))
              .build();
        }
      }
    }
    return getPutMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Get.Request,
      goblin.proto.Service.Get.Response> getGetMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Get",
      requestType = goblin.proto.Service.Get.Request.class,
      responseType = goblin.proto.Service.Get.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Get.Request,
      goblin.proto.Service.Get.Response> getGetMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Get.Request, goblin.proto.Service.Get.Response> getGetMethod;
    if ((getGetMethod = KVStoreGrpc.getGetMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getGetMethod = KVStoreGrpc.getGetMethod) == null) {
          KVStoreGrpc.getGetMethod = getGetMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Get.Request, goblin.proto.Service.Get.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Get"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Get.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Get.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Get"))
              .build();
        }
      }
    }
    return getGetMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Delete.Request,
      goblin.proto.Service.Delete.Response> getDeleteMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Delete",
      requestType = goblin.proto.Service.Delete.Request.class,
      responseType = goblin.proto.Service.Delete.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Delete.Request,
      goblin.proto.Service.Delete.Response> getDeleteMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Delete.Request, goblin.proto.Service.Delete.Response> getDeleteMethod;
    if ((getDeleteMethod = KVStoreGrpc.getDeleteMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getDeleteMethod = KVStoreGrpc.getDeleteMethod) == null) {
          KVStoreGrpc.getDeleteMethod = getDeleteMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Delete.Request, goblin.proto.Service.Delete.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Delete"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Delete.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Delete.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Delete"))
              .build();
        }
      }
    }
    return getDeleteMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.GenerateKV.Request,
      goblin.proto.Service.GenerateKV.Response> getGenerateKVMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "GenerateKV",
      requestType = goblin.proto.Service.GenerateKV.Request.class,
      responseType = goblin.proto.Service.GenerateKV.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.GenerateKV.Request,
      goblin.proto.Service.GenerateKV.Response> getGenerateKVMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.GenerateKV.Request, goblin.proto.Service.GenerateKV.Response> getGenerateKVMethod;
    if ((getGenerateKVMethod = KVStoreGrpc.getGenerateKVMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getGenerateKVMethod = KVStoreGrpc.getGenerateKVMethod) == null) {
          KVStoreGrpc.getGenerateKVMethod = getGenerateKVMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.GenerateKV.Request, goblin.proto.Service.GenerateKV.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "GenerateKV"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.GenerateKV.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.GenerateKV.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("GenerateKV"))
              .build();
        }
      }
    }
    return getGenerateKVMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.ExeBatch.Request,
      goblin.proto.Service.ExeBatch.Response> getExeBatchMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "ExeBatch",
      requestType = goblin.proto.Service.ExeBatch.Request.class,
      responseType = goblin.proto.Service.ExeBatch.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.ExeBatch.Request,
      goblin.proto.Service.ExeBatch.Response> getExeBatchMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.ExeBatch.Request, goblin.proto.Service.ExeBatch.Response> getExeBatchMethod;
    if ((getExeBatchMethod = KVStoreGrpc.getExeBatchMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getExeBatchMethod = KVStoreGrpc.getExeBatchMethod) == null) {
          KVStoreGrpc.getExeBatchMethod = getExeBatchMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.ExeBatch.Request, goblin.proto.Service.ExeBatch.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "ExeBatch"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.ExeBatch.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.ExeBatch.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("ExeBatch"))
              .build();
        }
      }
    }
    return getExeBatchMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.MigrateBatch.Request,
      goblin.proto.Service.MigrateBatch.Response> getMigrateBatchMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "MigrateBatch",
      requestType = goblin.proto.Service.MigrateBatch.Request.class,
      responseType = goblin.proto.Service.MigrateBatch.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.MigrateBatch.Request,
      goblin.proto.Service.MigrateBatch.Response> getMigrateBatchMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.MigrateBatch.Request, goblin.proto.Service.MigrateBatch.Response> getMigrateBatchMethod;
    if ((getMigrateBatchMethod = KVStoreGrpc.getMigrateBatchMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getMigrateBatchMethod = KVStoreGrpc.getMigrateBatchMethod) == null) {
          KVStoreGrpc.getMigrateBatchMethod = getMigrateBatchMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.MigrateBatch.Request, goblin.proto.Service.MigrateBatch.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "MigrateBatch"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.MigrateBatch.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.MigrateBatch.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("MigrateBatch"))
              .build();
        }
      }
    }
    return getMigrateBatchMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Watch.Request,
      goblin.proto.Service.Watch.Response> getWatchMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Watch",
      requestType = goblin.proto.Service.Watch.Request.class,
      responseType = goblin.proto.Service.Watch.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Watch.Request,
      goblin.proto.Service.Watch.Response> getWatchMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Watch.Request, goblin.proto.Service.Watch.Response> getWatchMethod;
    if ((getWatchMethod = KVStoreGrpc.getWatchMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getWatchMethod = KVStoreGrpc.getWatchMethod) == null) {
          KVStoreGrpc.getWatchMethod = getWatchMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Watch.Request, goblin.proto.Service.Watch.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.BIDI_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Watch"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Watch.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Watch.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Watch"))
              .build();
        }
      }
    }
    return getWatchMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.Transaction.Request,
      goblin.proto.Service.Transaction.Response> getTransactionMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Transaction",
      requestType = goblin.proto.Service.Transaction.Request.class,
      responseType = goblin.proto.Service.Transaction.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.Transaction.Request,
      goblin.proto.Service.Transaction.Response> getTransactionMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.Transaction.Request, goblin.proto.Service.Transaction.Response> getTransactionMethod;
    if ((getTransactionMethod = KVStoreGrpc.getTransactionMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getTransactionMethod = KVStoreGrpc.getTransactionMethod) == null) {
          KVStoreGrpc.getTransactionMethod = getTransactionMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.Transaction.Request, goblin.proto.Service.Transaction.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Transaction"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Transaction.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.Transaction.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("Transaction"))
              .build();
        }
      }
    }
    return getTransactionMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.StartMigration.Request,
      goblin.proto.Service.StartMigration.Response> getStartMigrationMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "StartMigration",
      requestType = goblin.proto.Service.StartMigration.Request.class,
      responseType = goblin.proto.Service.StartMigration.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.StartMigration.Request,
      goblin.proto.Service.StartMigration.Response> getStartMigrationMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.StartMigration.Request, goblin.proto.Service.StartMigration.Response> getStartMigrationMethod;
    if ((getStartMigrationMethod = KVStoreGrpc.getStartMigrationMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getStartMigrationMethod = KVStoreGrpc.getStartMigrationMethod) == null) {
          KVStoreGrpc.getStartMigrationMethod = getStartMigrationMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.StartMigration.Request, goblin.proto.Service.StartMigration.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "StartMigration"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.StartMigration.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.StartMigration.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("StartMigration"))
              .build();
        }
      }
    }
    return getStartMigrationMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Service.EndMigration.Request,
      goblin.proto.Service.EndMigration.Response> getEndMigrationMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "EndMigration",
      requestType = goblin.proto.Service.EndMigration.Request.class,
      responseType = goblin.proto.Service.EndMigration.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Service.EndMigration.Request,
      goblin.proto.Service.EndMigration.Response> getEndMigrationMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Service.EndMigration.Request, goblin.proto.Service.EndMigration.Response> getEndMigrationMethod;
    if ((getEndMigrationMethod = KVStoreGrpc.getEndMigrationMethod) == null) {
      synchronized (KVStoreGrpc.class) {
        if ((getEndMigrationMethod = KVStoreGrpc.getEndMigrationMethod) == null) {
          KVStoreGrpc.getEndMigrationMethod = getEndMigrationMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Service.EndMigration.Request, goblin.proto.Service.EndMigration.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "EndMigration"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.EndMigration.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Service.EndMigration.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVStoreMethodDescriptorSupplier("EndMigration"))
              .build();
        }
      }
    }
    return getEndMigrationMethod;
  }

  /**
   * Creates a new async stub that supports all call types for the service
   */
  public static KVStoreStub newStub(io.grpc.Channel channel) {
    return new KVStoreStub(channel);
  }

  /**
   * Creates a new blocking-style stub that supports unary and streaming output calls on the service
   */
  public static KVStoreBlockingStub newBlockingStub(
      io.grpc.Channel channel) {
    return new KVStoreBlockingStub(channel);
  }

  /**
   * Creates a new ListenableFuture-style stub that supports unary calls on the service
   */
  public static KVStoreFutureStub newFutureStub(
      io.grpc.Channel channel) {
    return new KVStoreFutureStub(channel);
  }

  /**
   */
  public static abstract class KVStoreImplBase implements io.grpc.BindableService {

    /**
     */
    public void connect(goblin.proto.Service.Connect.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Connect.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getConnectMethod(), responseObserver);
    }

    /**
     */
    public void put(goblin.proto.Service.Put.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Put.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getPutMethod(), responseObserver);
    }

    /**
     */
    public void get(goblin.proto.Service.Get.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Get.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getGetMethod(), responseObserver);
    }

    /**
     */
    public void delete(goblin.proto.Service.Delete.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Delete.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getDeleteMethod(), responseObserver);
    }

    /**
     * <pre>
     * generate kv according to custom logic
     * </pre>
     */
    public void generateKV(goblin.proto.Service.GenerateKV.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.GenerateKV.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getGenerateKVMethod(), responseObserver);
    }

    /**
     * <pre>
     * batch operations don't guarantee the order or automcity
     * </pre>
     */
    public void exeBatch(goblin.proto.Service.ExeBatch.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.ExeBatch.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getExeBatchMethod(), responseObserver);
    }

    /**
     * <pre>
     * used when shard migration
     * </pre>
     */
    public void migrateBatch(goblin.proto.Service.MigrateBatch.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.MigrateBatch.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getMigrateBatchMethod(), responseObserver);
    }

    /**
     * <pre>
     * we guarantee the following events to happen in order:
     * 1. writes/deletes happen to the watched keys
     * 2. watched changes is delivered to the client
     * 3. futher reads happen to the watch keys
     * which means when the client gets read response from #3, it should check if it receives changes from #2
     * </pre>
     */
    public io.grpc.stub.StreamObserver<goblin.proto.Service.Watch.Request> watch(
        io.grpc.stub.StreamObserver<goblin.proto.Service.Watch.Response> responseObserver) {
      return asyncUnimplementedStreamingCall(getWatchMethod(), responseObserver);
    }

    /**
     * <pre>
     * run a transaction with a bunch of read/write/deletes in order
     * these operations will either succeed or failed together
     * </pre>
     */
    public void transaction(goblin.proto.Service.Transaction.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Transaction.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getTransactionMethod(), responseObserver);
    }

    /**
     * <pre>
     **************** interactions with KVManager **************
     * </pre>
     */
    public void startMigration(goblin.proto.Service.StartMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.StartMigration.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getStartMigrationMethod(), responseObserver);
    }

    /**
     */
    public void endMigration(goblin.proto.Service.EndMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.EndMigration.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getEndMigrationMethod(), responseObserver);
    }

    @java.lang.Override public final io.grpc.ServerServiceDefinition bindService() {
      return io.grpc.ServerServiceDefinition.builder(getServiceDescriptor())
          .addMethod(
            getConnectMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.Connect.Request,
                goblin.proto.Service.Connect.Response>(
                  this, METHODID_CONNECT)))
          .addMethod(
            getPutMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.Put.Request,
                goblin.proto.Service.Put.Response>(
                  this, METHODID_PUT)))
          .addMethod(
            getGetMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.Get.Request,
                goblin.proto.Service.Get.Response>(
                  this, METHODID_GET)))
          .addMethod(
            getDeleteMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.Delete.Request,
                goblin.proto.Service.Delete.Response>(
                  this, METHODID_DELETE)))
          .addMethod(
            getGenerateKVMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.GenerateKV.Request,
                goblin.proto.Service.GenerateKV.Response>(
                  this, METHODID_GENERATE_KV)))
          .addMethod(
            getExeBatchMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.ExeBatch.Request,
                goblin.proto.Service.ExeBatch.Response>(
                  this, METHODID_EXE_BATCH)))
          .addMethod(
            getMigrateBatchMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.MigrateBatch.Request,
                goblin.proto.Service.MigrateBatch.Response>(
                  this, METHODID_MIGRATE_BATCH)))
          .addMethod(
            getWatchMethod(),
            asyncBidiStreamingCall(
              new MethodHandlers<
                goblin.proto.Service.Watch.Request,
                goblin.proto.Service.Watch.Response>(
                  this, METHODID_WATCH)))
          .addMethod(
            getTransactionMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.Transaction.Request,
                goblin.proto.Service.Transaction.Response>(
                  this, METHODID_TRANSACTION)))
          .addMethod(
            getStartMigrationMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.StartMigration.Request,
                goblin.proto.Service.StartMigration.Response>(
                  this, METHODID_START_MIGRATION)))
          .addMethod(
            getEndMigrationMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Service.EndMigration.Request,
                goblin.proto.Service.EndMigration.Response>(
                  this, METHODID_END_MIGRATION)))
          .build();
    }
  }

  /**
   */
  public static final class KVStoreStub extends io.grpc.stub.AbstractStub<KVStoreStub> {
    private KVStoreStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVStoreStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVStoreStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVStoreStub(channel, callOptions);
    }

    /**
     */
    public void connect(goblin.proto.Service.Connect.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Connect.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getConnectMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void put(goblin.proto.Service.Put.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Put.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getPutMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void get(goblin.proto.Service.Get.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Get.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getGetMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void delete(goblin.proto.Service.Delete.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Delete.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getDeleteMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * generate kv according to custom logic
     * </pre>
     */
    public void generateKV(goblin.proto.Service.GenerateKV.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.GenerateKV.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getGenerateKVMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * batch operations don't guarantee the order or automcity
     * </pre>
     */
    public void exeBatch(goblin.proto.Service.ExeBatch.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.ExeBatch.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getExeBatchMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * used when shard migration
     * </pre>
     */
    public void migrateBatch(goblin.proto.Service.MigrateBatch.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.MigrateBatch.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getMigrateBatchMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * we guarantee the following events to happen in order:
     * 1. writes/deletes happen to the watched keys
     * 2. watched changes is delivered to the client
     * 3. futher reads happen to the watch keys
     * which means when the client gets read response from #3, it should check if it receives changes from #2
     * </pre>
     */
    public io.grpc.stub.StreamObserver<goblin.proto.Service.Watch.Request> watch(
        io.grpc.stub.StreamObserver<goblin.proto.Service.Watch.Response> responseObserver) {
      return asyncBidiStreamingCall(
          getChannel().newCall(getWatchMethod(), getCallOptions()), responseObserver);
    }

    /**
     * <pre>
     * run a transaction with a bunch of read/write/deletes in order
     * these operations will either succeed or failed together
     * </pre>
     */
    public void transaction(goblin.proto.Service.Transaction.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.Transaction.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getTransactionMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     **************** interactions with KVManager **************
     * </pre>
     */
    public void startMigration(goblin.proto.Service.StartMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.StartMigration.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getStartMigrationMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void endMigration(goblin.proto.Service.EndMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Service.EndMigration.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getEndMigrationMethod(), getCallOptions()), request, responseObserver);
    }
  }

  /**
   */
  public static final class KVStoreBlockingStub extends io.grpc.stub.AbstractStub<KVStoreBlockingStub> {
    private KVStoreBlockingStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVStoreBlockingStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVStoreBlockingStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVStoreBlockingStub(channel, callOptions);
    }

    /**
     */
    public goblin.proto.Service.Connect.Response connect(goblin.proto.Service.Connect.Request request) {
      return blockingUnaryCall(
          getChannel(), getConnectMethod(), getCallOptions(), request);
    }

    /**
     */
    public goblin.proto.Service.Put.Response put(goblin.proto.Service.Put.Request request) {
      return blockingUnaryCall(
          getChannel(), getPutMethod(), getCallOptions(), request);
    }

    /**
     */
    public goblin.proto.Service.Get.Response get(goblin.proto.Service.Get.Request request) {
      return blockingUnaryCall(
          getChannel(), getGetMethod(), getCallOptions(), request);
    }

    /**
     */
    public goblin.proto.Service.Delete.Response delete(goblin.proto.Service.Delete.Request request) {
      return blockingUnaryCall(
          getChannel(), getDeleteMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * generate kv according to custom logic
     * </pre>
     */
    public goblin.proto.Service.GenerateKV.Response generateKV(goblin.proto.Service.GenerateKV.Request request) {
      return blockingUnaryCall(
          getChannel(), getGenerateKVMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * batch operations don't guarantee the order or automcity
     * </pre>
     */
    public goblin.proto.Service.ExeBatch.Response exeBatch(goblin.proto.Service.ExeBatch.Request request) {
      return blockingUnaryCall(
          getChannel(), getExeBatchMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * used when shard migration
     * </pre>
     */
    public goblin.proto.Service.MigrateBatch.Response migrateBatch(goblin.proto.Service.MigrateBatch.Request request) {
      return blockingUnaryCall(
          getChannel(), getMigrateBatchMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * run a transaction with a bunch of read/write/deletes in order
     * these operations will either succeed or failed together
     * </pre>
     */
    public goblin.proto.Service.Transaction.Response transaction(goblin.proto.Service.Transaction.Request request) {
      return blockingUnaryCall(
          getChannel(), getTransactionMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     **************** interactions with KVManager **************
     * </pre>
     */
    public goblin.proto.Service.StartMigration.Response startMigration(goblin.proto.Service.StartMigration.Request request) {
      return blockingUnaryCall(
          getChannel(), getStartMigrationMethod(), getCallOptions(), request);
    }

    /**
     */
    public goblin.proto.Service.EndMigration.Response endMigration(goblin.proto.Service.EndMigration.Request request) {
      return blockingUnaryCall(
          getChannel(), getEndMigrationMethod(), getCallOptions(), request);
    }
  }

  /**
   */
  public static final class KVStoreFutureStub extends io.grpc.stub.AbstractStub<KVStoreFutureStub> {
    private KVStoreFutureStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVStoreFutureStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVStoreFutureStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVStoreFutureStub(channel, callOptions);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.Connect.Response> connect(
        goblin.proto.Service.Connect.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getConnectMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.Put.Response> put(
        goblin.proto.Service.Put.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getPutMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.Get.Response> get(
        goblin.proto.Service.Get.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getGetMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.Delete.Response> delete(
        goblin.proto.Service.Delete.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getDeleteMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * generate kv according to custom logic
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.GenerateKV.Response> generateKV(
        goblin.proto.Service.GenerateKV.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getGenerateKVMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * batch operations don't guarantee the order or automcity
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.ExeBatch.Response> exeBatch(
        goblin.proto.Service.ExeBatch.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getExeBatchMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * used when shard migration
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.MigrateBatch.Response> migrateBatch(
        goblin.proto.Service.MigrateBatch.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getMigrateBatchMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * run a transaction with a bunch of read/write/deletes in order
     * these operations will either succeed or failed together
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.Transaction.Response> transaction(
        goblin.proto.Service.Transaction.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getTransactionMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     **************** interactions with KVManager **************
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.StartMigration.Response> startMigration(
        goblin.proto.Service.StartMigration.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getStartMigrationMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Service.EndMigration.Response> endMigration(
        goblin.proto.Service.EndMigration.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getEndMigrationMethod(), getCallOptions()), request);
    }
  }

  private static final int METHODID_CONNECT = 0;
  private static final int METHODID_PUT = 1;
  private static final int METHODID_GET = 2;
  private static final int METHODID_DELETE = 3;
  private static final int METHODID_GENERATE_KV = 4;
  private static final int METHODID_EXE_BATCH = 5;
  private static final int METHODID_MIGRATE_BATCH = 6;
  private static final int METHODID_TRANSACTION = 7;
  private static final int METHODID_START_MIGRATION = 8;
  private static final int METHODID_END_MIGRATION = 9;
  private static final int METHODID_WATCH = 10;

  private static final class MethodHandlers<Req, Resp> implements
      io.grpc.stub.ServerCalls.UnaryMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ServerStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ClientStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.BidiStreamingMethod<Req, Resp> {
    private final KVStoreImplBase serviceImpl;
    private final int methodId;

    MethodHandlers(KVStoreImplBase serviceImpl, int methodId) {
      this.serviceImpl = serviceImpl;
      this.methodId = methodId;
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public void invoke(Req request, io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_CONNECT:
          serviceImpl.connect((goblin.proto.Service.Connect.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Connect.Response>) responseObserver);
          break;
        case METHODID_PUT:
          serviceImpl.put((goblin.proto.Service.Put.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Put.Response>) responseObserver);
          break;
        case METHODID_GET:
          serviceImpl.get((goblin.proto.Service.Get.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Get.Response>) responseObserver);
          break;
        case METHODID_DELETE:
          serviceImpl.delete((goblin.proto.Service.Delete.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Delete.Response>) responseObserver);
          break;
        case METHODID_GENERATE_KV:
          serviceImpl.generateKV((goblin.proto.Service.GenerateKV.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.GenerateKV.Response>) responseObserver);
          break;
        case METHODID_EXE_BATCH:
          serviceImpl.exeBatch((goblin.proto.Service.ExeBatch.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.ExeBatch.Response>) responseObserver);
          break;
        case METHODID_MIGRATE_BATCH:
          serviceImpl.migrateBatch((goblin.proto.Service.MigrateBatch.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.MigrateBatch.Response>) responseObserver);
          break;
        case METHODID_TRANSACTION:
          serviceImpl.transaction((goblin.proto.Service.Transaction.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Transaction.Response>) responseObserver);
          break;
        case METHODID_START_MIGRATION:
          serviceImpl.startMigration((goblin.proto.Service.StartMigration.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.StartMigration.Response>) responseObserver);
          break;
        case METHODID_END_MIGRATION:
          serviceImpl.endMigration((goblin.proto.Service.EndMigration.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Service.EndMigration.Response>) responseObserver);
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
        case METHODID_WATCH:
          return (io.grpc.stub.StreamObserver<Req>) serviceImpl.watch(
              (io.grpc.stub.StreamObserver<goblin.proto.Service.Watch.Response>) responseObserver);
        default:
          throw new AssertionError();
      }
    }
  }

  private static abstract class KVStoreBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoFileDescriptorSupplier, io.grpc.protobuf.ProtoServiceDescriptorSupplier {
    KVStoreBaseDescriptorSupplier() {}

    @java.lang.Override
    public com.google.protobuf.Descriptors.FileDescriptor getFileDescriptor() {
      return goblin.proto.Service.getDescriptor();
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.ServiceDescriptor getServiceDescriptor() {
      return getFileDescriptor().findServiceByName("KVStore");
    }
  }

  private static final class KVStoreFileDescriptorSupplier
      extends KVStoreBaseDescriptorSupplier {
    KVStoreFileDescriptorSupplier() {}
  }

  private static final class KVStoreMethodDescriptorSupplier
      extends KVStoreBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoMethodDescriptorSupplier {
    private final String methodName;

    KVStoreMethodDescriptorSupplier(String methodName) {
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
      synchronized (KVStoreGrpc.class) {
        result = serviceDescriptor;
        if (result == null) {
          serviceDescriptor = result = io.grpc.ServiceDescriptor.newBuilder(SERVICE_NAME)
              .setSchemaDescriptor(new KVStoreFileDescriptorSupplier())
              .addMethod(getConnectMethod())
              .addMethod(getPutMethod())
              .addMethod(getGetMethod())
              .addMethod(getDeleteMethod())
              .addMethod(getGenerateKVMethod())
              .addMethod(getExeBatchMethod())
              .addMethod(getMigrateBatchMethod())
              .addMethod(getWatchMethod())
              .addMethod(getTransactionMethod())
              .addMethod(getStartMigrationMethod())
              .addMethod(getEndMigrationMethod())
              .build();
        }
      }
    }
    return result;
  }
}
