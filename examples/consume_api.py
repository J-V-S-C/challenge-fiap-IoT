"""Small Python client for the local C++ API. Run while the C++ server is up."""

import json
from urllib.request import Request, urlopen


BASE_URL = "http://127.0.0.1:8080/api/v1"


def call(method: str, path: str, body: dict | None = None) -> dict:
    payload = None if body is None else json.dumps(body).encode("utf-8")
    request = Request(
        BASE_URL + path,
        data=payload,
        headers={"Content-Type": "application/json"} if payload else {},
        method=method,
    )
    with urlopen(request, timeout=5) as response:
        return json.load(response)


def main() -> None:
    print("Saúde:", call("GET", "/health"))
    print("Casos:", call("GET", "/cases"))
    started = call("POST", "/simulations", {"time_limit_minutes": 30, "case_id": 2})
    simulation_id = started["simulation_id"]
    print("Início:", started)

    interaction = call(
        "POST",
        f"/simulations/{simulation_id}/interactions",
        {"student_text": "Offer water and monitor hydration.", "elapsed_minutes": 2},
    )
    print("Interação:", interaction)
    print("Estado consultado:", call("GET", f"/simulations/{simulation_id}"))

    call("POST", f"/simulations/{simulation_id}/finish", {})
    print("Resultado:", call("GET", f"/simulations/{simulation_id}/result"))


if __name__ == "__main__":
    main()
