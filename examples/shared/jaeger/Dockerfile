FROM jaegertracing/all-in-one@sha256:712801515fb163e5b7f3406e51b0561c957192e85e33d84247426cb4e301db3c
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
