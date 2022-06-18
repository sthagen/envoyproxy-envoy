#!/bin/bash

# Reformat API protos to canonical proto style using protoxform.

set -e

read -ra BAZEL_BUILD_OPTIONS <<< "${BAZEL_BUILD_OPTIONS:-}"


[[ "$1" == "check" || "$1" == "fix" || "$1" == "freeze" ]] || {
    echo "Usage: $0 <check|fix|freeze>"
    exit 1
}

# Developers working on protoxform and other proto format tooling changes will need to override the
# following check by setting FORCE_PROTO_FORMAT=yes in the environment.
./tools/git/modified_since_last_github_commit.sh ./api/envoy proto || \
    [[ "${FORCE_PROTO_FORMAT}" == "yes" ]] || {
        echo "Skipping proto_format.sh due to no API change"
        exit 0
    }

# Generate //versioning:active_protos.
./tools/proto_format/active_protos_gen.py ./api > ./api/versioning/BUILD

# If the specified command is 'freeze', we tell protoxform to adjust package version status to
# reflect a major version freeze and then do a regular 'fix'.
PROTO_SYNC_CMD="$1"
if [[ "$1" == "freeze" ]]; then
    declare -r FREEZE_ARG="--//tools/api_proto_plugin:extra_args=freeze"
    PROTO_SYNC_CMD="fix"
fi

# Invoke protoxform aspect.
bazel build "${BAZEL_BUILD_OPTIONS[@]}" \
    --//tools/api_proto_plugin:default_type_db_target=@envoy_api//:all_protos \
    ${FREEZE_ARG} \
    //tools/protoxform:api_protoxform

# Copy back the FileDescriptorProtos that protoxform emitted to the source tree. This involves
# pretty-printing to format with protoprint.
bazel run //tools/proto_format:proto_sync -- "--mode=${PROTO_SYNC_CMD}" --ci

# Need to regenerate //versioning:active_protos before building type DB below if freezing.
if [[ "$1" == "freeze" ]]; then
    ./tools/proto_format/active_protos_gen.py ./api > ./api/versioning/BUILD
fi

# Generate api/BUILD file based on updated type database.
bazel build "${BAZEL_BUILD_OPTIONS[@]}" //tools/type_whisperer:api_build_file
cp -f bazel-bin/tools/type_whisperer/BUILD.api_build_file api/BUILD
