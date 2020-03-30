package sidecar

import (
  "context"
  "fmt"
  "google.golang.org/grpc"
  sidecar "sidecar/generated"
  "time"
)

type Role uint16

const (
  Leader    Role = 1
  Follower  Role = 2
  Candidate Role = 3
)

type Meta struct {
  role        Role
  term        uint64
  lastIndex   uint64
  commitIndex uint64
  leaderHint  string
}

func GetMeta() {
  address := "127.0.0.1:50055"
  conn, err := grpc.Dial(address, grpc.WithInsecure())
  if err != nil {
    fmt.Println("Error %s", err)
    return
  }
  fmt.Println("established connection with %s", address)
  defer conn.Close()
  c := sidecar.NewInteropReadMetaServiceClient(conn)
  ctx, cancel := context.WithTimeout(context.Background(), time.Duration(10)*time.Second)
  defer cancel()
  r, err := c.ReadMeta(ctx, &sidecar.ReadMeta_Request{})
  if err != nil {
    fmt.Println("call grpc truncatePrefix has error %v", err)
    return
  }
  fmt.Println("Hello Sidecar, term=%d, last_index=%d", r.Term, r.LastIndex)
}
