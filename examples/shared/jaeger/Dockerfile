FROM jaegertracing/all-in-one@sha256:7fe25eee01afde68cdd481ab5bbe0c158599b9d86c81f9cf44630ab8a9eec94e
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
