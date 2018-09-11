package main;

import java.io.IOException;
import java.math.BigInteger;
import java.util.ArrayList;

import com.sonycsl.echo.eoj.device.airconditioner.HomeAirConditioner;
import com.sonycsl.echo.eoj.device.housingfacilities.GeneralLighting;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

import com.sonycsl.echo.eoj.device.DeviceObject;

public class MainClass {

	public static final int GROUPCODE_HOUSING_FACILITIES = EchonetController.ConvertByteToInt(new byte[]{(byte) 0x02}) ;
	public static final int CLASSCODE_GENERAL_LIGHTING = EchonetController.ConvertByteToInt(new byte[]{(byte) 0x90}) ;

	public static String NAME_USER;
	public static String PASSWORD;
	public static int homeID = 1;

	public static MqttConnection mqttCon1;
	public static MqttConnection mqttCon2;

	public static void main(String[] args) throws IOException {
		//String[] args = { "hoangbd.info", "hoangbd", "123456" };

		System.out.println("Started");

//		String hostMQTT = args[0];
//		String usernameMQTT = args[1];
//		String passwordMQTT = args[2];
		String hostMQTT = "localhost";
		String usernameMQTT = "IOTplf";
		String passwordMQTT = "123456789";

		mqttCon1 = new MqttConnection(hostMQTT);
		mqttCon2 = new MqttConnection(hostMQTT);
		NAME_USER = usernameMQTT;
		PASSWORD = passwordMQTT;

		MqttConnectOptions optionsConnect = new MqttConnectOptions();
		optionsConnect.setUserName(NAME_USER);
		optionsConnect.setPassword(PASSWORD.toCharArray());

        mqttCon1.Connect(optionsConnect);
		EchonetController controller = new EchonetController();
		mqttCon2.Connect(optionsConnect);
		mqttCon2.Subcribe("user/" + NAME_USER + "/" + homeID + "/+/+/+/command");
		//user/username/homeID/room/deviceType/ipNode/command

		mqttCon2.mMqttClient.setCallback(new MqttCallback() {

			@Override
			public void messageArrived(String topic, MqttMessage message) throws Exception {
				System.out.println("Message : " + message.toString());
				System.out.println("Recieved message...");

				try {
					//Topic format: user/username/homeID/room/deviceType/ipNode/command
					String[] modifyTopic = topic.split("/");
					int lengthMTopic = modifyTopic.length;
					String[] ipNodeAndInstance = modifyTopic[lengthMTopic-2].split(".");
					String ipNode = ipNodeAndInstance[0];
					int instanceCode = Integer.valueOf(ipNodeAndInstance[1]);
					String deviceType = modifyTopic[lengthMTopic-3];

					for (DeviceObject dev : controller.mDevices){
						System.out.println(deviceType);
						System.out.println(dev.getNode().getAddress().getHostAddress() + " " + ipNode);

						if(dev.getNode().getAddress().getHostAddress().equals(ipNode)
								&& EchonetController.ConvertByteToInt(new byte[]{dev.getInstanceCode()}) == instanceCode
									/*&& convertFromStringDevToCode(deviceType).toClassCode ==  dev.getClassCode()..*/){
							// chu y moi loai
							// thiet bi trong mot node se co list instance tuong ung tu 0 -> den thiet bi cuoi cung loai
							// voi thiet bi do nen co the co truong hop node chua 4 thiet bi den 4 thiet bi dieu hoa
							// chung co cung NodeIp, thiet bi den thu nhat co InstanceCode = 0 , dieu hoa thu nhat cung
							// co instancode = 0 nen neu chi kiem tra ipNode va instanceCode la chua du -> de viec kiem
							// tra instancde co tac dung thi phai kiem tra dung loai device nua
							// tuy nhien do da thong nhat 1 node chi chua 1 LOAI thiet bi ( co nhieu device nhung cung loai)
							// nen ko can dung ham "convert..." de kiem tra thiet bi tuong ung nua
							JSONParser parserMsg = new JSONParser();
							JSONObject jsonObjMsg = (JSONObject) parserMsg.parse(message.toString());
							JsonMqttHandle modifyJsonMess = new JsonMqttHandle();
							ArrayList<JsonMqttHandle.JsonKeyAndValue> jsonContent = modifyJsonMess.getJsonObject(jsonObjMsg);

							for(JsonMqttHandle.JsonKeyAndValue jkv : jsonContent){
								String epcStr = jkv.key.split("x")[1];// dua 0xD5 ve dang D5
								String edtStr = jkv.value.split("x")[1];
								long edtLong = Long.parseLong(edtStr, 16); // AA0F245C int 2853119068 bit 1010 1010 0000 1111 0010 0100 0101 1100
								//System.out.println(BigInteger.valueOf(decode2).toByteArray());
								int epcInt = Integer.parseInt(epcStr,16); // chuyen Hex(16) String Sang INT
								byte[] edtByteArr = BigInteger.valueOf(edtLong).toByteArray();
								byte epcByte = Integer.valueOf(epcInt).byteValue();
								dev.set().reqSetProperty(epcByte,edtByteArr);
							}

						}
					}

				} catch (Exception e) {
					e.printStackTrace();
				}

			}


			@Override
			public void deliveryComplete(IMqttDeliveryToken token) {

			}

			@Override
			public void connectionLost(Throwable cause) {
				System.out.println("Lost MQTT Connection" + cause.getMessage());
			}
		});

	}



}
