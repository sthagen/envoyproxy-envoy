FROM jaegertracing/all-in-one@sha256:c3ebfc07f6dd0854a45dd67f617410fc74b4b06e50ced331db2c95a7bc48b1f0
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
