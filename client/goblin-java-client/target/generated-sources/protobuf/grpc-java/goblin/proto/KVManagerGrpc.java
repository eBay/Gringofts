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
    comments = "Source: control.proto")
public final class KVManagerGrpc {

  private KVManagerGrpc() {}

  public static final String SERVICE_NAME = "goblin.proto.KVManager";

  // Static method descriptors that strictly reflect the proto.
  private static volatile io.grpc.MethodDescriptor<goblin.proto.Control.Router.Request,
      goblin.proto.Control.Router.Response> getRouterMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "Router",
      requestType = goblin.proto.Control.Router.Request.class,
      responseType = goblin.proto.Control.Router.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Control.Router.Request,
      goblin.proto.Control.Router.Response> getRouterMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Control.Router.Request, goblin.proto.Control.Router.Response> getRouterMethod;
    if ((getRouterMethod = KVManagerGrpc.getRouterMethod) == null) {
      synchronized (KVManagerGrpc.class) {
        if ((getRouterMethod = KVManagerGrpc.getRouterMethod) == null) {
          KVManagerGrpc.getRouterMethod = getRouterMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Control.Router.Request, goblin.proto.Control.Router.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "Router"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.Router.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.Router.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVManagerMethodDescriptorSupplier("Router"))
              .build();
        }
      }
    }
    return getRouterMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Control.OnStartup.Request,
      goblin.proto.Control.OnStartup.Response> getOnStartupMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "OnStartup",
      requestType = goblin.proto.Control.OnStartup.Request.class,
      responseType = goblin.proto.Control.OnStartup.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Control.OnStartup.Request,
      goblin.proto.Control.OnStartup.Response> getOnStartupMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Control.OnStartup.Request, goblin.proto.Control.OnStartup.Response> getOnStartupMethod;
    if ((getOnStartupMethod = KVManagerGrpc.getOnStartupMethod) == null) {
      synchronized (KVManagerGrpc.class) {
        if ((getOnStartupMethod = KVManagerGrpc.getOnStartupMethod) == null) {
          KVManagerGrpc.getOnStartupMethod = getOnStartupMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Control.OnStartup.Request, goblin.proto.Control.OnStartup.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "OnStartup"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.OnStartup.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.OnStartup.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVManagerMethodDescriptorSupplier("OnStartup"))
              .build();
        }
      }
    }
    return getOnStartupMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Control.OnMigration.Request,
      goblin.proto.Control.OnMigration.Response> getOnMigrationMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "OnMigration",
      requestType = goblin.proto.Control.OnMigration.Request.class,
      responseType = goblin.proto.Control.OnMigration.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Control.OnMigration.Request,
      goblin.proto.Control.OnMigration.Response> getOnMigrationMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Control.OnMigration.Request, goblin.proto.Control.OnMigration.Response> getOnMigrationMethod;
    if ((getOnMigrationMethod = KVManagerGrpc.getOnMigrationMethod) == null) {
      synchronized (KVManagerGrpc.class) {
        if ((getOnMigrationMethod = KVManagerGrpc.getOnMigrationMethod) == null) {
          KVManagerGrpc.getOnMigrationMethod = getOnMigrationMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Control.OnMigration.Request, goblin.proto.Control.OnMigration.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "OnMigration"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.OnMigration.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.OnMigration.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVManagerMethodDescriptorSupplier("OnMigration"))
              .build();
        }
      }
    }
    return getOnMigrationMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Control.AddCluster.Request,
      goblin.proto.Control.AddCluster.Response> getAddClusterMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "AddCluster",
      requestType = goblin.proto.Control.AddCluster.Request.class,
      responseType = goblin.proto.Control.AddCluster.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Control.AddCluster.Request,
      goblin.proto.Control.AddCluster.Response> getAddClusterMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Control.AddCluster.Request, goblin.proto.Control.AddCluster.Response> getAddClusterMethod;
    if ((getAddClusterMethod = KVManagerGrpc.getAddClusterMethod) == null) {
      synchronized (KVManagerGrpc.class) {
        if ((getAddClusterMethod = KVManagerGrpc.getAddClusterMethod) == null) {
          KVManagerGrpc.getAddClusterMethod = getAddClusterMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Control.AddCluster.Request, goblin.proto.Control.AddCluster.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "AddCluster"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.AddCluster.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.AddCluster.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVManagerMethodDescriptorSupplier("AddCluster"))
              .build();
        }
      }
    }
    return getAddClusterMethod;
  }

  private static volatile io.grpc.MethodDescriptor<goblin.proto.Control.RemoveCluster.Request,
      goblin.proto.Control.RemoveCluster.Response> getRemoveClusterMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "RemoveCluster",
      requestType = goblin.proto.Control.RemoveCluster.Request.class,
      responseType = goblin.proto.Control.RemoveCluster.Response.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<goblin.proto.Control.RemoveCluster.Request,
      goblin.proto.Control.RemoveCluster.Response> getRemoveClusterMethod() {
    io.grpc.MethodDescriptor<goblin.proto.Control.RemoveCluster.Request, goblin.proto.Control.RemoveCluster.Response> getRemoveClusterMethod;
    if ((getRemoveClusterMethod = KVManagerGrpc.getRemoveClusterMethod) == null) {
      synchronized (KVManagerGrpc.class) {
        if ((getRemoveClusterMethod = KVManagerGrpc.getRemoveClusterMethod) == null) {
          KVManagerGrpc.getRemoveClusterMethod = getRemoveClusterMethod =
              io.grpc.MethodDescriptor.<goblin.proto.Control.RemoveCluster.Request, goblin.proto.Control.RemoveCluster.Response>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "RemoveCluster"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.RemoveCluster.Request.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  goblin.proto.Control.RemoveCluster.Response.getDefaultInstance()))
              .setSchemaDescriptor(new KVManagerMethodDescriptorSupplier("RemoveCluster"))
              .build();
        }
      }
    }
    return getRemoveClusterMethod;
  }

  /**
   * Creates a new async stub that supports all call types for the service
   */
  public static KVManagerStub newStub(io.grpc.Channel channel) {
    return new KVManagerStub(channel);
  }

  /**
   * Creates a new blocking-style stub that supports unary and streaming output calls on the service
   */
  public static KVManagerBlockingStub newBlockingStub(
      io.grpc.Channel channel) {
    return new KVManagerBlockingStub(channel);
  }

  /**
   * Creates a new ListenableFuture-style stub that supports unary calls on the service
   */
  public static KVManagerFutureStub newFutureStub(
      io.grpc.Channel channel) {
    return new KVManagerFutureStub(channel);
  }

  /**
   */
  public static abstract class KVManagerImplBase implements io.grpc.BindableService {

    /**
     * <pre>
     * get routing info, invoked either by clients or clusters
     * </pre>
     */
    public void router(goblin.proto.Control.Router.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.Router.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getRouterMethod(), responseObserver);
    }

    /**
     * <pre>
     * get correct status and necessary info when cluster is on power
     * </pre>
     */
    public void onStartup(goblin.proto.Control.OnStartup.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.OnStartup.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getOnStartupMethod(), responseObserver);
    }

    /**
     * <pre>
     * report migration status
     * </pre>
     */
    public void onMigration(goblin.proto.Control.OnMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.OnMigration.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getOnMigrationMethod(), responseObserver);
    }

    /**
     * <pre>
     * TODO: make these automatically without manual efforts
     * </pre>
     */
    public void addCluster(goblin.proto.Control.AddCluster.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.AddCluster.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getAddClusterMethod(), responseObserver);
    }

    /**
     */
    public void removeCluster(goblin.proto.Control.RemoveCluster.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.RemoveCluster.Response> responseObserver) {
      asyncUnimplementedUnaryCall(getRemoveClusterMethod(), responseObserver);
    }

    @java.lang.Override public final io.grpc.ServerServiceDefinition bindService() {
      return io.grpc.ServerServiceDefinition.builder(getServiceDescriptor())
          .addMethod(
            getRouterMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Control.Router.Request,
                goblin.proto.Control.Router.Response>(
                  this, METHODID_ROUTER)))
          .addMethod(
            getOnStartupMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Control.OnStartup.Request,
                goblin.proto.Control.OnStartup.Response>(
                  this, METHODID_ON_STARTUP)))
          .addMethod(
            getOnMigrationMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Control.OnMigration.Request,
                goblin.proto.Control.OnMigration.Response>(
                  this, METHODID_ON_MIGRATION)))
          .addMethod(
            getAddClusterMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Control.AddCluster.Request,
                goblin.proto.Control.AddCluster.Response>(
                  this, METHODID_ADD_CLUSTER)))
          .addMethod(
            getRemoveClusterMethod(),
            asyncUnaryCall(
              new MethodHandlers<
                goblin.proto.Control.RemoveCluster.Request,
                goblin.proto.Control.RemoveCluster.Response>(
                  this, METHODID_REMOVE_CLUSTER)))
          .build();
    }
  }

  /**
   */
  public static final class KVManagerStub extends io.grpc.stub.AbstractStub<KVManagerStub> {
    private KVManagerStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVManagerStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVManagerStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVManagerStub(channel, callOptions);
    }

    /**
     * <pre>
     * get routing info, invoked either by clients or clusters
     * </pre>
     */
    public void router(goblin.proto.Control.Router.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.Router.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getRouterMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * get correct status and necessary info when cluster is on power
     * </pre>
     */
    public void onStartup(goblin.proto.Control.OnStartup.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.OnStartup.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getOnStartupMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * report migration status
     * </pre>
     */
    public void onMigration(goblin.proto.Control.OnMigration.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.OnMigration.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getOnMigrationMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * TODO: make these automatically without manual efforts
     * </pre>
     */
    public void addCluster(goblin.proto.Control.AddCluster.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.AddCluster.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getAddClusterMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void removeCluster(goblin.proto.Control.RemoveCluster.Request request,
        io.grpc.stub.StreamObserver<goblin.proto.Control.RemoveCluster.Response> responseObserver) {
      asyncUnaryCall(
          getChannel().newCall(getRemoveClusterMethod(), getCallOptions()), request, responseObserver);
    }
  }

  /**
   */
  public static final class KVManagerBlockingStub extends io.grpc.stub.AbstractStub<KVManagerBlockingStub> {
    private KVManagerBlockingStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVManagerBlockingStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVManagerBlockingStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVManagerBlockingStub(channel, callOptions);
    }

    /**
     * <pre>
     * get routing info, invoked either by clients or clusters
     * </pre>
     */
    public goblin.proto.Control.Router.Response router(goblin.proto.Control.Router.Request request) {
      return blockingUnaryCall(
          getChannel(), getRouterMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * get correct status and necessary info when cluster is on power
     * </pre>
     */
    public goblin.proto.Control.OnStartup.Response onStartup(goblin.proto.Control.OnStartup.Request request) {
      return blockingUnaryCall(
          getChannel(), getOnStartupMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * report migration status
     * </pre>
     */
    public goblin.proto.Control.OnMigration.Response onMigration(goblin.proto.Control.OnMigration.Request request) {
      return blockingUnaryCall(
          getChannel(), getOnMigrationMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * TODO: make these automatically without manual efforts
     * </pre>
     */
    public goblin.proto.Control.AddCluster.Response addCluster(goblin.proto.Control.AddCluster.Request request) {
      return blockingUnaryCall(
          getChannel(), getAddClusterMethod(), getCallOptions(), request);
    }

    /**
     */
    public goblin.proto.Control.RemoveCluster.Response removeCluster(goblin.proto.Control.RemoveCluster.Request request) {
      return blockingUnaryCall(
          getChannel(), getRemoveClusterMethod(), getCallOptions(), request);
    }
  }

  /**
   */
  public static final class KVManagerFutureStub extends io.grpc.stub.AbstractStub<KVManagerFutureStub> {
    private KVManagerFutureStub(io.grpc.Channel channel) {
      super(channel);
    }

    private KVManagerFutureStub(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected KVManagerFutureStub build(io.grpc.Channel channel,
        io.grpc.CallOptions callOptions) {
      return new KVManagerFutureStub(channel, callOptions);
    }

    /**
     * <pre>
     * get routing info, invoked either by clients or clusters
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Control.Router.Response> router(
        goblin.proto.Control.Router.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getRouterMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * get correct status and necessary info when cluster is on power
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Control.OnStartup.Response> onStartup(
        goblin.proto.Control.OnStartup.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getOnStartupMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * report migration status
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Control.OnMigration.Response> onMigration(
        goblin.proto.Control.OnMigration.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getOnMigrationMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * TODO: make these automatically without manual efforts
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Control.AddCluster.Response> addCluster(
        goblin.proto.Control.AddCluster.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getAddClusterMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<goblin.proto.Control.RemoveCluster.Response> removeCluster(
        goblin.proto.Control.RemoveCluster.Request request) {
      return futureUnaryCall(
          getChannel().newCall(getRemoveClusterMethod(), getCallOptions()), request);
    }
  }

  private static final int METHODID_ROUTER = 0;
  private static final int METHODID_ON_STARTUP = 1;
  private static final int METHODID_ON_MIGRATION = 2;
  private static final int METHODID_ADD_CLUSTER = 3;
  private static final int METHODID_REMOVE_CLUSTER = 4;

  private static final class MethodHandlers<Req, Resp> implements
      io.grpc.stub.ServerCalls.UnaryMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ServerStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ClientStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.BidiStreamingMethod<Req, Resp> {
    private final KVManagerImplBase serviceImpl;
    private final int methodId;

    MethodHandlers(KVManagerImplBase serviceImpl, int methodId) {
      this.serviceImpl = serviceImpl;
      this.methodId = methodId;
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public void invoke(Req request, io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_ROUTER:
          serviceImpl.router((goblin.proto.Control.Router.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Control.Router.Response>) responseObserver);
          break;
        case METHODID_ON_STARTUP:
          serviceImpl.onStartup((goblin.proto.Control.OnStartup.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Control.OnStartup.Response>) responseObserver);
          break;
        case METHODID_ON_MIGRATION:
          serviceImpl.onMigration((goblin.proto.Control.OnMigration.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Control.OnMigration.Response>) responseObserver);
          break;
        case METHODID_ADD_CLUSTER:
          serviceImpl.addCluster((goblin.proto.Control.AddCluster.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Control.AddCluster.Response>) responseObserver);
          break;
        case METHODID_REMOVE_CLUSTER:
          serviceImpl.removeCluster((goblin.proto.Control.RemoveCluster.Request) request,
              (io.grpc.stub.StreamObserver<goblin.proto.Control.RemoveCluster.Response>) responseObserver);
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
        default:
          throw new AssertionError();
      }
    }
  }

  private static abstract class KVManagerBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoFileDescriptorSupplier, io.grpc.protobuf.ProtoServiceDescriptorSupplier {
    KVManagerBaseDescriptorSupplier() {}

    @java.lang.Override
    public com.google.protobuf.Descriptors.FileDescriptor getFileDescriptor() {
      return goblin.proto.Control.getDescriptor();
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.ServiceDescriptor getServiceDescriptor() {
      return getFileDescriptor().findServiceByName("KVManager");
    }
  }

  private static final class KVManagerFileDescriptorSupplier
      extends KVManagerBaseDescriptorSupplier {
    KVManagerFileDescriptorSupplier() {}
  }

  private static final class KVManagerMethodDescriptorSupplier
      extends KVManagerBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoMethodDescriptorSupplier {
    private final String methodName;

    KVManagerMethodDescriptorSupplier(String methodName) {
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
      synchronized (KVManagerGrpc.class) {
        result = serviceDescriptor;
        if (result == null) {
          serviceDescriptor = result = io.grpc.ServiceDescriptor.newBuilder(SERVICE_NAME)
              .setSchemaDescriptor(new KVManagerFileDescriptorSupplier())
              .addMethod(getRouterMethod())
              .addMethod(getOnStartupMethod())
              .addMethod(getOnMigrationMethod())
              .addMethod(getAddClusterMethod())
              .addMethod(getRemoveClusterMethod())
              .build();
        }
      }
    }
    return result;
  }
}
