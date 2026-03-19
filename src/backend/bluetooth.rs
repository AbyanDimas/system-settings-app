use crate::error::AppError;
use zbus::Connection;
use zbus::zvariant::{Value, OwnedValue};
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct BluetoothDevice {
    pub mac: String,
    pub name: String,
    pub connected: bool,
    pub paired: bool,
    pub icon: String,
    pub object_path: String,
    pub battery_percentage: Option<u8>,
    pub rssi: Option<i16>,
}

#[derive(Debug, Clone)]
pub struct BluetoothAdapter {
    pub name: String,
    pub powered: bool,
    pub discoverable: bool,
    pub discovering: bool,
    pub address: String,
    pub modalias: String,
    pub pairable: bool,
}

#[derive(Clone)]
pub struct BluetoothService {}

impl BluetoothService {
    pub fn new() -> Result<Self, AppError> {
        Ok(Self {})
    }

    async fn get_property(conn: &Connection, prop: &str) -> Option<OwnedValue> {
        conn.call_method(
            Some("org.bluez"),
            "/org/bluez/hci0",
            Some("org.freedesktop.DBus.Properties"),
            "Get",
            &("org.bluez.Adapter1", prop),
        ).await.ok()?.body().deserialize::<OwnedValue>().ok()
    }

    pub async fn get_adapter_info(&self) -> Result<BluetoothAdapter, AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        
        let alias_val = Self::get_property(&conn, "Alias").await.unwrap_or_else(|| OwnedValue::try_from(Value::from("Unknown")).unwrap());
        let powered_val = Self::get_property(&conn, "Powered").await.unwrap_or_else(|| OwnedValue::try_from(Value::from(false)).unwrap());
        let discovering_val = Self::get_property(&conn, "Discovering").await.unwrap_or_else(|| OwnedValue::try_from(Value::from(false)).unwrap());
        let address_val = Self::get_property(&conn, "Address").await.unwrap_or_else(|| OwnedValue::try_from(Value::from("00:00:00:00:00:00")).unwrap());
        let modalias_val = Self::get_property(&conn, "Modalias").await.unwrap_or_else(|| OwnedValue::try_from(Value::from("")).unwrap());
        let pairable_val = Self::get_property(&conn, "Pairable").await.unwrap_or_else(|| OwnedValue::try_from(Value::from(false)).unwrap());
        let discoverable_val = Self::get_property(&conn, "Discoverable").await.unwrap_or_else(|| OwnedValue::try_from(Value::from(false)).unwrap());

        let name = Value::from(alias_val).downcast::<String>().unwrap_or_else(|_| "Unknown".into());
        let is_powered = Value::from(powered_val).downcast::<bool>().unwrap_or(false);
        let is_discovering = Value::from(discovering_val).downcast::<bool>().unwrap_or(false);
        let address = Value::from(address_val).downcast::<String>().unwrap_or_else(|_| "00:00:00:00:00:00".into());
        let modalias = Value::from(modalias_val).downcast::<String>().unwrap_or_else(|_| "".into());
        let is_pairable = Value::from(pairable_val).downcast::<bool>().unwrap_or(false);
        let is_discoverable = Value::from(discoverable_val).downcast::<bool>().unwrap_or(false);

        Ok(BluetoothAdapter {
            name,
            powered: is_powered,
            discoverable: is_discoverable,
            discovering: is_discovering,
            address,
            modalias,
            pairable: is_pairable,
        })
    }

    pub async fn set_adapter_alias(&self, name: &str) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        conn.call_method(
            Some("org.bluez"),
            "/org/bluez/hci0",
            Some("org.freedesktop.DBus.Properties"),
            "Set",
            &("org.bluez.Adapter1", "Alias", Value::from(name.to_string())),
        ).await.map_err(|e| AppError::DBus(e.to_string()))?;
        Ok(())
    }

    pub async fn start_discovery(&self) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        let _ = conn.call_method(
            Some("org.bluez"),
            "/org/bluez/hci0",
            Some("org.bluez.Adapter1"),
            "StartDiscovery",
            &(),
        ).await;
        Ok(())
    }

    pub async fn stop_discovery(&self) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        let _ = conn.call_method(
            Some("org.bluez"),
            "/org/bluez/hci0",
            Some("org.bluez.Adapter1"),
            "StopDiscovery",
            &(),
        ).await;
        Ok(())
    }

    pub async fn get_devices(&self) -> Result<Vec<BluetoothDevice>, AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        
        let managed_objects: HashMap<zbus::zvariant::OwnedObjectPath, HashMap<String, HashMap<String, OwnedValue>>> = conn.call_method(
            Some("org.bluez"),
            "/",
            Some("org.freedesktop.DBus.ObjectManager"),
            "GetManagedObjects",
            &(),
        ).await.map_err(|e| AppError::DBus(e.to_string()))?
        .body().deserialize().map_err(|e| AppError::DBus(e.to_string()))?;

        let mut devices = Vec::new();

        for (path, interfaces) in managed_objects {
            if let Some(props) = interfaces.get("org.bluez.Device1") {
                let name = props.get("Alias")
                    .and_then(|v| v.downcast_ref::<String>().ok().map(|s| s.clone()))
                    .or_else(|| props.get("Name").and_then(|v| v.downcast_ref::<String>().ok().map(|s| s.clone())))
                    .unwrap_or_else(|| "Unknown Device".to_string());
                
                let mac = props.get("Address")
                    .and_then(|v| v.downcast_ref::<String>().ok().map(|s| s.clone()))
                    .unwrap_or_else(|| "Unknown".to_string());

                let connected = props.get("Connected")
                    .and_then(|v| v.downcast_ref::<bool>().ok())
                    .unwrap_or(false);

                let paired = props.get("Paired")
                    .and_then(|v| v.downcast_ref::<bool>().ok())
                    .unwrap_or(false);

                let icon = props.get("Icon")
                    .and_then(|v| v.downcast_ref::<String>().ok().map(|s| s.clone()))
                    .unwrap_or_else(|| "".to_string());

                let rssi = props.get("RSSI")
                    .and_then(|v| v.downcast_ref::<i16>().ok());

                let battery_percentage = interfaces.get("org.bluez.Battery1")
                    .and_then(|battery_props| battery_percentage_from_props(battery_props));

                devices.push(BluetoothDevice {
                    mac,
                    name,
                    connected,
                    paired,
                    icon,
                    object_path: path.to_string(),
                    battery_percentage,
                    rssi,
                });
            }
        }

        Ok(devices)
    }

    pub async fn set_powered(&self, powered: bool) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        conn.call_method(
            Some("org.bluez"),
            "/org/bluez/hci0",
            Some("org.freedesktop.DBus.Properties"),
            "Set",
            &("org.bluez.Adapter1", "Powered", Value::from(powered)),
        ).await.map_err(|e| AppError::DBus(e.to_string()))?;
        
        Ok(())
    }

    pub async fn connect_device(&self, path: &str) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        
        match conn.call_method(
            Some("org.bluez"),
            path,
            Some("org.bluez.Device1"),
            "Connect",
            &(),
        ).await {
            Ok(_) => Ok(()),
            Err(e) => {
                if e.to_string().contains("NotPaired") || e.to_string().contains("Authentication") {
                    let _ = conn.call_method(
                        Some("org.bluez"),
                        path,
                        Some("org.bluez.Device1"),
                        "Pair",
                        &(),
                    ).await;
                    
                    conn.call_method(
                        Some("org.bluez"),
                        path,
                        Some("org.bluez.Device1"),
                        "Connect",
                        &(),
                    ).await.map_err(|e| AppError::DBus(e.to_string()))?;
                    Ok(())
                } else {
                    Err(AppError::DBus(e.to_string()))
                }
            }
        }
    }

    pub async fn disconnect_device(&self, path: &str) -> Result<(), AppError> {
        let conn = Connection::system().await.map_err(|e| AppError::DBus(e.to_string()))?;
        conn.call_method(
            Some("org.bluez"),
            path,
            Some("org.bluez.Device1"),
            "Disconnect",
            &(),
        ).await.map_err(|e| AppError::DBus(e.to_string()))?;
        Ok(())
    }

    pub fn is_auto_start_enabled(&self) -> bool {
        let home = std::env::var("HOME").unwrap_or_default();
        let path = std::path::PathBuf::from(home).join(".config/hypr/autostart.conf");
        if let Ok(content) = std::fs::read_to_string(&path) {
            content.lines().any(|l| l.contains("rfkill unblock bluetooth"))
        } else {
            false
        }
    }

    pub fn set_auto_start(&self, enabled: bool) -> Result<(), AppError> {
        let home = std::env::var("HOME").unwrap_or_default();
        let path = std::path::PathBuf::from(home).join(".config/hypr/autostart.conf");
        let mut lines: Vec<String> = if path.exists() {
            std::fs::read_to_string(&path)
                .unwrap_or_default()
                .lines()
                .map(|s| s.to_string())
                .filter(|l| !l.contains("rfkill unblock bluetooth") && !l.contains("rfkill block bluetooth"))
                .collect()
        } else {
            Vec::new()
        };

        if enabled {
            lines.push("exec-once = rfkill unblock bluetooth".to_string());
        } else {
            lines.push("exec-once = rfkill block bluetooth".to_string());
        }

        std::fs::write(&path, lines.join("\n") + "\n").map_err(|e| AppError::Unknown(e.to_string()))?;
        Ok(())
    }
}

fn battery_percentage_from_props(props: &HashMap<String, OwnedValue>) -> Option<u8> {
    props.get("Percentage")
        .and_then(|v| {
            u8::try_from(v).ok()
                .or_else(|| u16::try_from(v).ok().map(|v| v as u8))
        })
}
