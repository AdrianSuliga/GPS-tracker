import asyncio
import aiocoap
import socket
import aiocoap.resource as resource

REMOTE_GNSS_URL = "coap://californium.eclipseprojects.io/large-update"
REMOTE_GEO_URL = "coap://californium.eclipseprojects.io/validate"
POLL_INTERVAL = 5


class SimpleResource(resource.Resource):
    def __init__(self, name):
        super().__init__()
        self.name = name
        self.value = b""

    async def render_get(self, request):
        return aiocoap.Message(payload=self.value)


def get_lan_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except Exception:
        return "127.0.0.1"
    finally:
        s.close()


async def fetch_gnss(context):
    request = aiocoap.Message(code=aiocoap.GET, uri=REMOTE_GNSS_URL)
    response = await context.request(request).response
    return response.payload.decode("utf-8", errors="replace")

async def fetch_geo(context):
    request = aiocoap.Message(code=aiocoap.GET, uri=REMOTE_GEO_URL)
    response = await context.request(request).response
    return response.payload.decode("utf-8", errors="replace")

async def poll(context, gnss, geo):
    last_gnss = None
    last_geo = None

    while True:
        try:
            payload_gnss = await fetch_gnss(context)

            if payload_gnss != last_gnss:
                last_gnss = payload_gnss
                gnss.value = last_gnss.encode("utf-8")

                print("New GNSS data put")

            payload_geo = await fetch_geo(context)

            if payload_geo != last_geo:
                last_geo = payload_geo
                geo.value = last_geo.encode("utf-8")

                print("New GEO data put")

        except Exception as e:
            pass

        await asyncio.sleep(POLL_INTERVAL)


async def main():
    root = resource.Site()

    gnss = SimpleResource("gnss")
    geo = SimpleResource("geo")

    root.add_resource(["gnss"], gnss)
    root.add_resource(["geo"], geo)

    ip = get_lan_ip()
    port = 5683

    await aiocoap.Context.create_server_context(
        root,
        bind=("0.0.0.0", port)
    )

    client_context = await aiocoap.Context.create_client_context()

    print("Server running")
    print(f"coap://{ip}:{port}/gnss")
    print(f"coap://{ip}:{port}/geo")

    asyncio.create_task(poll(client_context, gnss, geo))

    await asyncio.get_running_loop().create_future()


if __name__ == "__main__":
    asyncio.run(main())
