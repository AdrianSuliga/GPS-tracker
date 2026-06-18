import asyncio
from aiocoap import *

COAP_SERVER = "coap://192.168.18.9"

GNSS_RESOURCE = "gnss"
GEO_RESOURCE = "geo"

path_to_str = {
    GNSS_RESOURCE: "GPS",
    GEO_RESOURCE: "LTE geolocation"
}

RESOURCES = [GNSS_RESOURCE, GEO_RESOURCE]

REQUEST_TIMEOUT = 5

async def fetch_resource(protocol, resource_path):
    url = f"{COAP_SERVER}/{resource_path}"

    request = Message(code=GET, uri=url)

    try:
        response = await asyncio.wait_for(
            protocol.request(request).response,
            timeout=REQUEST_TIMEOUT
        )

        print(f"\n[OK] {path_to_str[resource_path]}")
        print(f"Payload: {response.payload.decode('utf-8')}")

    except asyncio.TimeoutError:
        print(f"\n[TIMEOUT] {resource_path}")

    except Exception as e:
        print(f"\n[ERROR] {resource_path}")
        print(f"Reason: {e}")

async def main():
    protocol = await Context.create_client_context()

    await asyncio.sleep(1)

    tasks = [
        fetch_resource(protocol, resource)
        for resource in RESOURCES
    ]

    await asyncio.gather(*tasks)

if __name__ == "__main__":
    asyncio.run(main())
