package com.example.dog;

public class UserInformation {
    public double latitude;
    public double longitude;
    public float temperature;
    public float purse;
    public String name;
    public String memo;
    public String age;
    public String imageUrl;
    public double beforelat;
    public double beforelon;
    public double user_latitude;
    public double user_longitude;



    public UserInformation(){
    }

    public UserInformation(double latitude, double longitude, float temperature, float purse,
                           String age, String name, String memo, String imageUrl){
        this.latitude = latitude;
        this.longitude = longitude;
        this.temperature = temperature;
        this.purse = purse;
        this.name = name;
        this.age = age;
        this.memo = memo;
        this.imageUrl = imageUrl;
    }
}
