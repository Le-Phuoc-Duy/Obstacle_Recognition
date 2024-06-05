part of 'mapdelivery_bloc.dart';

@immutable
abstract class MapdeliveryEvent {}

class OnMapReadyEvent extends MapdeliveryEvent {}

class OnMarkertsDeliveryEvent extends MapdeliveryEvent {

  final LatLng location;
  final LatLng destination;

  OnMarkertsDeliveryEvent(this.location, this.destination);
}

class OnMarkRouteDeliveryEvent extends MapdeliveryEvent {}

class OnEmitLocationDeliveryEvent extends MapdeliveryEvent {
  final String id;
  final LatLng location;

  OnEmitLocationDeliveryEvent(this.id, this.location);
}

