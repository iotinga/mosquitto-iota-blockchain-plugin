![Finanziato dall'Unione europea | Ministero dell'Università e della Ricerca | Italia domani PNRR | iNEST ](assets/HEADER_INEST.png)

>Programma iNEST Codice ECS00000043 finanziato sui fondi PNRR MUR – M4C2” – Investimento 1.5. Avviso “Ecosistemi dell’Innovazione” 
>CUP progetto B33D24000100004

# MQTT Mosquitto IOTA Blockchain plugin

This plugin enhances data integrity inserting the message payload on the IOTA blockchain and then appending the Block ID to the message.

## Building

To compile the project first install the dependencies and then run the following commands:

```bash
mkdir build && cd build
cmake ..
make
```

Then you can install the library in the `/usr/local/lib` path with the command:

```bash
sudo make install
```

### Dependencies

This plugin has been tested with the following dependencies:

- Eclipse Mosquitto v2.0.18 (https://github.com/eclipse/mosquitto)
- IOTA SDK python3 bindings v1.1.4 (https://pypi.org/project/iota-sdk/)

## License

This project is licensed under the Apache License 2.0 - see [LICENSE](LICENSE) file for details.
