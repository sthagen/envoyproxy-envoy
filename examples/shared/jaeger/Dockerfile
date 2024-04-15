FROM jaegertracing/all-in-one@sha256:eecd1da1bab411d58d41573dae008473b6204ffaf2943af5ae49b115efc662c9
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
