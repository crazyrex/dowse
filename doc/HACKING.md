# Event channels

In Dowse the 'dynamic' redis database (indexed by `src/database.h`
macros) is gathering a number of live events occurring across its
namespaces, so that clients can subscribe to one or more channels and
handle these events.

Inside the dynamic database the following channels are present:

- dns_query_channel (produced by dnscap/plugin/dowse)
- pgl_info_channel  (produced by pgld.c)

As the listener API shapes up, this namespace may change in the close
future. Here below some examples for commandline use:

## DNS

To subscribe to DNS events from CLI, do from the dowse source root:

```shell
db=`awk '/db_dynamic/ { print $3 }' src/database.h`
cat <<EOF | redis-cli -n $db --raw
SUBSCRIBE dns_query_channel
EOF
```

## PGL (pgl_info_channel)

To subscribe to PeerGuardian's events from CLI:

```shell
db=`awk '/db_dynamic/ { print $3 }' src/database.h`
cat <<EOF | redis-cli -n $db --raw
SUBSCRIBE pgl_info_channel
EOF
```