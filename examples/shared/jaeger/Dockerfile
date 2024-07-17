FROM jaegertracing/all-in-one@sha256:19923874f6d66dbcb94f02f9665d98e9e96d942f51ac171d38e2a98f4f0eef00
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
