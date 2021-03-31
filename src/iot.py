def has_property_changed(msg, prop):
    if "desired" not in msg["state"].keys() or "reported" not in msg["state"].keys():
        print("Desired state not present")
        return False
    if prop not in msg["state"]["desired"].keys():
        print("{} is not in desired".format(prop))
        return False
    if (
        prop in msg["state"]["reported"].keys()
        and msg["state"]["desired"][prop] == msg["state"]["reported"][prop]
    ):
        print("{} does not require an update".format(prop))
        return False

    return True


def is_in_sync(msg, prop, expected):
    if (
        "reported" not in msg["state"].keys()
        or prop not in msg["state"]["reported"].keys()
        or msg["state"]["reported"][prop] != expected
    ):
        return False
    return True
