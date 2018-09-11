package main;

import java.io.IOException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.json.JSONException;
import org.json.simple.JSONObject;

import com.sonycsl.echo.Echo;
import com.sonycsl.echo.EchoProperty;
import com.sonycsl.echo.eoj.EchoObject;
import com.sonycsl.echo.eoj.device.DeviceObject;
import com.sonycsl.echo.eoj.device.cookinghousehold.ElectricHotWaterPot;
import com.sonycsl.echo.eoj.device.housingfacilities.GeneralLighting;
import com.sonycsl.echo.eoj.device.managementoperation.Switch;
import com.sonycsl.echo.eoj.device.sensor.ElectricEnergySensor;
import com.sonycsl.echo.eoj.device.sensor.HumiditySensor;
import com.sonycsl.echo.eoj.device.sensor.IlluminanceSensor;
import com.sonycsl.echo.eoj.profile.NodeProfile;
import com.sonycsl.echo.processing.defaults.DefaultController;
import com.sonycsl.echo.processing.defaults.DefaultNodeProfile;

public class EchonetController {
    ArrayList<DeviceObject> mDevices;
    ArrayList<DeviceObject> mLightingDevices;

    EchonetController() throws IOException {
        mDevices = new ArrayList<DeviceObject>();
        mLightingDevices = new ArrayList<>();

        try {
            Echo.start(new DefaultNodeProfile(), new DeviceObject[]{new DefaultController()});
            NodeProfile.informG().reqInformInstanceListNotification().send();
            Echo.addEventListener(new Echo.EventListener() {

                // Execute when have new General Lighting device
                @Override
                public void onNewGeneralLighting(GeneralLighting device) {
                    super.onNewGeneralLighting(device);
                    System.out.println("New General Lighting found");
                    mDevices.add(device);
                    mLightingDevices.add(device);
                    String ipNode = device.getNode().getAddress().getHostAddress();
                    org.json.JSONObject jsonMessage = new org.json.JSONObject();
                    try {
                        jsonMessage.put("node_ip", device.getNode().getAddress().getHostAddress());
                        jsonMessage.put("group_code", device.getClassGroupCode());
                        jsonMessage.put("class_code", device.getClassCode());
                        jsonMessage.put("instance_code", device.getInstanceCode());
                        jsonMessage.put("power_limit", 0);
                        jsonMessage.put("OperationStatus", 0);
                        jsonMessage.put("MeidumCapacity:", 0.0);
                        jsonMessage.put("Location:", "bedroom");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }

                    device.setReceiver(new GeneralLighting.Receiver() {
                        protected void onGetOperationStatus(EchoObject eoj, short tid, byte esv, EchoProperty property,
                                                            boolean success) {
                            super.onGetOperationStatus(eoj, tid, esv, property, success);
                            if (!success) {
                                System.out.println("Can't get Value");
                                return;

                            }
                            try {
                                // jsonMessage.put("PowerLimited", 0);
                                jsonMessage.remove("OperationStatus");
                                if (property.edt[0] == 48) {
                                    System.out.println("Operation Status: true");
                                    jsonMessage.put("OperationStatus", true);
                                } else if (property.edt[0] == 49) {
                                    System.out.println("Operation Status: false");
                                    jsonMessage.put("OperationStatus", false);
                                } else {
                                    System.out.println("Operation Status: false");
                                    jsonMessage.put("OperationStatus", false);
                                }

                            } catch (JSONException ex) {
                                ex.printStackTrace();
                            }
                        }

                        protected void onGetPowerLimitSetting(EchoObject eoj, short tid, byte esv, EchoProperty property, boolean success) {
                            super.onGetPowerLimitSetting(eoj, tid, esv, property, success);
                            if (!success) {
                                return;
                            }
//                            int high = property.edt[1] >= 0 ? property.edt[1] : 256 + property.edt[1];
//                            int low = property.edt[0] >= 0 ? property.edt[0] : 256 + property.edt[0];
//                            int powerLimit = low | (high << 8);
                            int powerLimit = ConvertByteToInt(property.edt);
                            try {
                                System.out.println("Power Limit:  " + powerLimit);
                                jsonMessage.remove("power_limit");
                                jsonMessage.put("power_limit", powerLimit);
                            } catch (Exception e) {

                            }
                        }

                        protected void onGetInstallationLocation(EchoObject eoj, short tid, byte esv, EchoProperty property, boolean success) {
                            super.onGetInstallationLocation(eoj, tid, esv, property, success);
                            if (!success)
                            {
                                return;
                            }
                            int locationValue = property.edt[0];

                            String location = "bedroom";
                            if (locationValue >= 8 && locationValue <=15)
                            {
                                location = "living room";
                            } else if (locationValue >= 16 && locationValue <= 23)
                            {
                                location = "dining room";
                            } else if (locationValue >= 24 && locationValue <= 31)
                            {
                                location = "kitchen";
                            } else if (locationValue >= 32 && locationValue <= 39)
                            {
                                location = "bathroom";
                            } else if (locationValue >= 64 && locationValue >= 71)
                            {
                                location = "bedroom";
                            }

                            try{
                                System.out.println("Location: " + location);
                                jsonMessage.remove("Location");
                                jsonMessage.put("Location", location);
                            } catch (Exception e)
                            {

                            }
                        }

                        protected void onGetMediumCapacity(EchoObject eoj, short tid, byte esv, EchoProperty property, boolean success) {
                            super.onGetMediumCapacity(eoj, tid, esv, property, success);
                            //System.out.println("MediumCapacity: not found");
                            if (!success)
                            {
                                return;
                            }

                            float mediumCapacity = ConvertByteToFloat(property.edt);
                            try{
                                System.out.println("Medium Capacity: " + mediumCapacity);
                                jsonMessage.remove("MediumCapacity");
                                jsonMessage.put("MediumCapacity", mediumCapacity);
                            } catch (Exception e) {

                            }
                        }
                    });
                    Runnable r = () -> {
                        while (true) {

                            try {
                                Thread.sleep(3000);
                                System.out.println("SEND REQUEST OF MULTIPLE PROPERTIES...");
                                //device.get().reqGetOperationStatus().send();
                                device.get().reqGetOperationStatus().reqGetPowerLimitSetting().reqGetInstallationLocation().reqGetProperty((byte) 0xE1).send();
                                MainClass.mqttCon1.PublishMessage(jsonMessage.toString(), "user/" + MainClass.NAME_USER + "/" + MainClass.homeID +
                                        "/bedroom/lighting_device/" + ipNode + "/data");
//                                    System.out.println(jsonMessage.toString());
                            } catch (IOException | InterruptedException e) {
                                // TODO Auto-generated catch block
                                e.printStackTrace();
                            }
                        }
                    };
                    new Thread(r).start();
                }
            });

        } catch (

                IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        NodeProfile.getG().reqGetNumberOfSelfNodeInstances().send();
       // NodeProfile.informG().reqInformInstanceListNotification().send();


        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public static float ConvertByteToFloat(byte[] b) {
        ByteBuffer bb = ByteBuffer.wrap(b);
        float value = bb.getFloat();
        return value;
    }
    public static int ConvertByteToInt(byte[] b)
    {
        int value= 0;
        for(int i=0;i<b.length;i++){
            int n=(b[i]<0?(int)b[i]+256:(int)b[i])<<(8*i);
            value+=n;
        }
        return value;
    }
    public static byte getClassGroupCode(short code ) {
        return (byte)((code >> 8) & 0xFF);
    }

    public static byte getClassCode(short code ) {
        return (byte)(code & 0xFF);
    }
}