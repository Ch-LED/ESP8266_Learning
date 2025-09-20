import asyncio
from aioconsole import ainput
import websockets
import json
import time
import uuid
from enum import Enum
from typing import Dict, List, Any, Callable

class EventType(Enum):
    MESSAGE_RECEIVED = "message_received"
    TIME_SYNC_RESPONSE = "time_sync_response"
    PONG_RECEIVED = "pong_received"
    COMMAND_SENT = "command_sent"

class EventBus:
    def __init__(self):
        self.handlers: Dict[EventType, List[Callable]] = {}

    def subscribe(self, event_type: EventType, handler: Callable):
        if event_type not in self.handlers:
            self.handlers[event_type] = []
        self.handlers[event_type].append(handler)

    async def publish(self, event_type: EventType, data: Any = None):
        if event_type in self.handlers:
            for handler in self.handlers[event_type]:
                if asyncio.iscoroutinefunction(handler):
                    asyncio.create_task(handler(data))
                else:
                    handler(data)

bus = EventBus()

output_queue = asyncio.Queue(maxsize=100)

pending_pings: Dict[str, float] = {}
pending_syncs: Dict[str, float] = {}

time_offset = 0.0

async def output_processor():
    while True:
        try:
            message = await asyncio.wait_for(output_queue.get(), timeout=1.0)
            print(message)
            output_queue.task_done()
        except asyncio.TimeoutError:
            continue
        except asyncio.CancelledError:
            break
        except Exception as e:
            print(f"Error in output processor: {e}")
            break

async def enqueue_output(message: str):
    try:
        await output_queue.put(message)
    except asyncio.QueueFull:
        print(f"Output queue is full. Dropping message: {message}")

async def receive_messages(websocket):
    try:
        sync_id = str(uuid.uuid4())
        sync_send_time = time.time()
        sync_message = {
            "type": "time_sync_request",
            "id": sync_id,
            "server_send_time": sync_send_time
        }
        await websocket.send(json.dumps(sync_message))
        pending_syncs[sync_id] = sync_send_time
        await enqueue_output(f"Sent time sync request: {sync_id}")

        async for message in websocket:
            try:
                data = json.loads(message)
                msg_type = data.get("type")

                if msg_type == "pong":
                    ping_id = data.get("id")
                    ping_timestamp = data.get("ping_timestamp")
                    client_timestamp = data.get("client_timestamp")

                    await bus.publish(EventType.PONG_RECEIVED, {
                        "id": ping_id,
                        "ping_timestamp": ping_timestamp,
                        "client_timestamp": client_timestamp,
                        "server_timestamp": time.time(),
                        "data": data.get("data", {})
                    })

                elif msg_type == "time_sync_response":
                    await bus.publish(EventType.TIME_SYNC_RESPONSE, {
                        "id": data.get("id"),
                        "client_send_time": data.get("client_send_time"),
                        "client_receive_time": data.get("client_receive_time"),
                        "server_receive_time": time.time()
                    })

                else:
                    await bus.publish(EventType.MESSAGE_RECEIVED, {
                        "type": msg_type,
                        "content": data.get("content", ""),
                        "timestamp": data.get("timestamp", time.time()),
                        "data": data.get("data", {}),
                        "raw": data
                    })
            except json.JSONDecodeError:
                await enqueue_output(f"Received non-JSON message: {message}")
            except Exception as e:
                await enqueue_output(f"Error processing message: {e}")
    except websockets.ConnectionClosed:
        await enqueue_output("Connection closed")
    except asyncio.CancelledError:
        await enqueue_output("Receiver task cancelled")

async def handle_pong_received(data):
    ping_id = data["id"]
    ping_timestamp = data["ping_timestamp"]
    client_timestamp = data["client_timestamp"]
    server_receive_time = data["server_timestamp"]
    
    corrected_client_timestamp = client_timestamp + time_offset
    
    total_delay = (server_receive_time - ping_timestamp)
    
    if ping_id in pending_pings:
        del pending_pings[ping_id]

    if client_timestamp is not None:
        network_delay = (server_receive_time - corrected_client_timestamp)
        processing_delay = (corrected_client_timestamp - ping_timestamp)
        message = f"[PONG] ID: {ping_id}, Total Delay: {total_delay:.3f}ms, Network Delay: {network_delay:.3f}ms, Processing Delay: {processing_delay:.3f}ms"
    else:
        message = f"[PONG] ID: {ping_id}, Total Delay: {total_delay:.3f}ms"

    await enqueue_output(message)

async def handle_message_received(data):
    msg_type = data["type"]
    content = data["content"]
    timestamp = data["timestamp"]
    message = f"[MESSAGE] Type: {msg_type}, Content: {content}, Timestamp: {timestamp}"

    await enqueue_output(message)

async def handle_time_sync_response(data):
    sync_id = data["id"]
    client_send_time = data["client_send_time"]
    client_receive_time = data["client_receive_time"]
    server_receive_time = data["server_receive_time"]

    if sync_id in pending_syncs:
        server_send_time = pending_syncs[sync_id]
        del pending_syncs[sync_id]

        theta = ((client_receive_time - server_send_time) + (client_send_time - server_receive_time)) / 2
        delta = (server_receive_time - server_send_time) - (client_send_time - client_receive_time)
        
        message = f"[TIME SYNC] ID: {sync_id}\n"
        message += f"  Server Send Time: {server_send_time}\n"
        message +=f"  Client Receive Time: {client_receive_time}\n"
        message +=f"  Client Send Time: {client_send_time}\n"
        message +=f"  Server Receive Time: {server_receive_time}\n"
        message +=f"  Theta (Offset): {-theta:.6f}s\n"
        message +=f"  Delta (RTT): {delta:.6f}s\n"

        global time_offset
        time_offset = -theta

        await enqueue_output(message)

async def event_handler(websocket):
    await enqueue_output("Connection established.")

    bus.subscribe(EventType.PONG_RECEIVED, handle_pong_received)
    bus.subscribe(EventType.MESSAGE_RECEIVED, handle_message_received)
    bus.subscribe(EventType.TIME_SYNC_RESPONSE, handle_time_sync_response)

    receiver_task = asyncio.create_task(receive_messages(websocket))

    try:
        while True:
            op = await ainput("[>] ")
            command = op.split(" -")[0]
            data = op.split(" -")[1:] if " -" in op else ""

            if command.lower() in {"exit", "quit"}:
                await enqueue_output("Exiting...")
                break
            elif command.lower() == "cls":
                print("\033c", end="")
            elif command.lower() == "ping":
                ping_id = str(uuid.uuid4())
                ping_timestamp = time.time()

                ping_message = {
                    "type": "ping",
                    "id": ping_id,
                    "ping_timestamp": ping_timestamp,
                    "data": {
                        "message": "Ping request"
                    }
                }

                await websocket.send(json.dumps(ping_message))

                pending_pings[ping_id] = ping_timestamp

                await enqueue_output(f"[PING] Sent ping with ID: {ping_id}")
            elif command.lower() == "help":
                help_text = """
                Available commands:
                ping               - Send a ping to the server
                exit, quit         - Exit the application
                cls                - Clear the console
                help               - Show this help message
                info               - Show client info
                reboot             - Send a reboot command to the client
                set led -<on/off>   - Set LED state
                get mock_sensor    - Get mock sensor data
                """
                await enqueue_output(help_text)
            elif command.strip():
                command_message = {
                    "type": "command",
                    "content": command,
                    "timestamp": time.time(),
                    "data": {
                        "arguments": data
                    }
                }

                await websocket.send(json.dumps(command_message))
    except websockets.ConnectionClosed:
        await enqueue_output("Connection closed.")
    except asyncio.CancelledError:
        await enqueue_output("Handler cancelled.")
    finally:
        receiver_task.cancel()
        await websocket.close()

async def main():
    async with websockets.serve(event_handler, "0.0.0.0", 8765):
        await enqueue_output("WebSocket server started on ws://0.0.0.0:8765")
        output_task = asyncio.create_task(output_processor())
        try:
            await asyncio.Future()
        finally:
            output_task.cancel()
            try:
                await output_task
            except asyncio.CancelledError:
                pass

if __name__ == "__main__":
    asyncio.run(main())